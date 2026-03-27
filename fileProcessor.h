#pragma once

#include <QObject>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qtimer.h>
#include <atomic>
#include "modes.h"
#include <QThreadPool>
#include <QString>

class fileProcessor  : public QObject
{
    Q_OBJECT

public:
    fileProcessor(QObject *parent = nullptr);
    ~fileProcessor();

    void setSaveMode(saveMode SaveMode){this->SaveMode = SaveMode;}
    void setStartMode(startMode StartMode){this->StartMode = StartMode;}
    void setDeleteMode(bool deleteMode){this->deleteMode = deleteMode;}
    void setTargetDirectory(const QString& targetDirectory) {this->targetDirectory = targetDirectory;}
    void setXorKey(const QByteArray& key){this->key = key;}

    bool processFile(const QString& filePath){ return processOneFile(filePath);}

    bool isWorking() const { return workingNow.load(); }
    bool isLastTask() const { return filesLeft == 0;}

    void stop() {qDebug() << "пользовательский стоп"; stopReq.store(true); }
    void resetStopFlag() {stopReq.store(false); }
    void resetReportedFlag() {reported.store(false);}
    void emitFinished();
    void taskCompleted(){ --filesLeft; }
    void reportStopReason();

public slots:
    void run(const QStringList& files);

signals:
    void error(const QString& info);
    void stopped();
    void progress(qint64 processed, qint64 total);
    void finished(startMode StartMode);

private:
    QThreadPool* threadPool;
    static thread_local QByteArray buffer;
    const int buffSize = 2 * 1024 * 1024;

    bool deleteMode = false;
    saveMode SaveMode;
    startMode StartMode;
    QString targetDirectory;
    QByteArray key;

    std::atomic<qint64> bytesProcessed;
    std::atomic<qint64> totalSize;
    std::atomic<int> filesLeft;
    std::atomic<bool> stopReq = false;
    std::atomic<bool> workingNow = false;
    std::atomic<bool> fileErr = false;
    std::atomic<bool> reported = false;
    mutable std::atomic<int> fileNum;

    QString generatePath(const QString& fileName) const;
    void processFiles(const QStringList& files);
    bool processOneFile(const QString& path);
};
