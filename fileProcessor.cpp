#include "fileProcessor.h"
#include "fileProcess.h"
#include <QApplication>
#include <QThread>

fileProcessor::fileProcessor(QObject *parent)
    : QObject(parent) {
    threadPool = new QThreadPool(this);
    threadPool->setMaxThreadCount(4);
}

thread_local QByteArray fileProcessor::buffer;

QString fileProcessor::generatePath(const QString& fileName) const {
    QFileInfo fileInfo(fileName);
    QString fileSuffix = fileInfo.completeSuffix();
    QString fileBaseName = fileInfo.completeBaseName();

    QString currFilePath = QDir(targetDirectory).filePath(fileName);
    QString tmpFilePath = currFilePath + ".tmp";

    if (SaveMode == saveMode::overwrite) return currFilePath;
    if (!QFile::exists(currFilePath) && !QFile::exists(tmpFilePath)) return currFilePath;

    fileNum.store(0);
    while (fileNum < 50000) {
        QString newFileName = QString("%1_%2.%3").arg(fileBaseName).arg(++fileNum).arg(fileSuffix);
        QString newPath = QDir(targetDirectory).filePath(newFileName);
        tmpFilePath = newPath + ".tmp";
        if (!QFile::exists(newPath) && !QFile::exists(tmpFilePath)) return newPath;
    }

    QString currDate = QDateTime::currentDateTime().toString("dd-MM-yyyy_hh-mm-ss");
    QString datePath = QDir(targetDirectory).filePath(QString("%1_%2.%3").arg(fileBaseName).arg(currDate).arg(fileSuffix));
    return datePath;//если за адекватное колво попыток не нашелся свободный номер, сохранить с текущей датой
}

void fileProcessor::processFiles(const QStringList& files){
    if (files.isEmpty()) return;

    filesLeft.store(files.size());
    bytesProcessed.store(0);
    qint64 size = 0;

    for (const auto& file : files) {
        QFileInfo fileInfo(file);
        size += fileInfo.size();
    }
    totalSize.store(size);

    QFileInfo info(files.first());
    QString dirFrom = info.absolutePath();
    bool sameDir = dirFrom == targetDirectory;
    bool needToDelete = deleteMode && !(sameDir && SaveMode == saveMode::overwrite);

    for (const auto& file : files) {
        if (fileErr.load() || stopReq.load()){
            reportStopReason();
            return;
        }
        fileProcess* task = new fileProcess(file, this, needToDelete);
        threadPool->start(task);
    }
}

bool fileProcessor::processOneFile(const QString& filePath){
    QFile fileFrom(filePath);
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    QString outputPath = generatePath(fileName);
    QString tmpPath = outputPath + ".tmp";
    QFile tmpFile(tmpPath);

    if (!fileFrom.open(QIODevice::ReadOnly)) {
        fileErr.store(true);
        return false;
    }
    if (!tmpFile.open(QIODevice::WriteOnly)) {
        fileErr.store(true);
        fileFrom.close();
        tmpFile.remove();
        return false;
    }

    if (buffer.size() != buffSize){
        buffer.resize(buffSize);
        qDebug() << "ресайз буфера, поток " << QThread::currentThreadId();
    }
    char* buffPtr = buffer.data();

    int bytesRead;
    while ((bytesRead = fileFrom.read(buffPtr, buffSize)) > 0) {
        if (fileErr.load() || stopReq.load()) { //нужно проверять оба условия т.к. ошибка/остановка могла быть вызвана в других потоках
            tmpFile.remove();
            return false;
        }
        for (int i = 0; i < bytesRead; ++i) {
            buffPtr[i] ^= key[i % 8]; //размер буфера кратен 8, смещения не будет
        }
        tmpFile.write(buffPtr, bytesRead);
        bytesProcessed += bytesRead;
        emit progress(bytesProcessed.load(), totalSize.load());
    }

    fileFrom.close();
    tmpFile.close();

    QFile::remove(outputPath);//на случай если стоит режим перезаписи. Без удаления tmp может пытаться переименоваться в существующий файл
    if (!tmpFile.rename(outputPath)) {
        fileErr.store(true);
        tmpFile.remove();
        return false;
    }

    return true;
}

void fileProcessor::reportStopReason(){
    if (reported.exchange(true)) return;//от переполнения очереди сигналов. Сообщаем только о первой остановке
    if (stopReq.load()) emit stopped();
    else emit error(QString("При обработке возникла ошибка. Не обработано файлов: %1").arg(filesLeft.load()));

    workingNow.store(false);
    qDebug() << "закончили работу";
}

void fileProcessor::emitFinished(){
    emit finished(StartMode);

    workingNow.store(false);
    qDebug() << "закончили работу";
}

void fileProcessor::run(const QStringList& files){
    if (files.empty()){
        workingNow.store(false);
        qDebug() << "файлов нет, флаг сброшен";
        return;
    }
    fileErr.store(false);

    workingNow.store(true);
    qDebug() << "начали работу";
    processFiles(files);
}

fileProcessor::~fileProcessor(){
    threadPool->waitForDone();
}

