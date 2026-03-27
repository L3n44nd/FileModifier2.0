#include "fileModifier.h"
#include <QtWidgets/QFileDialog>
#include <qbuttongroup.h>
#include <qmessagebox.h>

fileModifier::fileModifier(QWidget *parent)
    : QWidget(parent)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this](){
        emitRunSignal(startMode::regular);
    });

    proc = new fileProcessor(this);
    connect(this, &fileModifier::runProcessor, proc, &fileProcessor::run);

    setupUI();
}

void fileModifier::setupUI() {
    ui.setupUi(this);
    ui.xorField->setInputMask("HH-HH-HH-HH-HH-HH-HH-HH");

    QButtonGroup* startModeGroup = new QButtonGroup(this);
    startModeGroup->addButton(ui.singleRbtn);
    startModeGroup->addButton(ui.regularRbtn);

    QButtonGroup* saveModeGroup = new QButtonGroup(this);
    saveModeGroup->addButton(ui.createNewRbtn);
    saveModeGroup->addButton(ui.overwriteRbtn);

    connect(ui.dirSelectBtn, &QPushButton::clicked, this, [this]() {
        selectDirBtnClicked(ui.dirFromField);
    });
    connect(ui.dirSelectBtn2, &QPushButton::clicked, this, [this]() {
        selectDirBtnClicked(ui.dirToField);
    });
    connect(ui.regularRbtn, &QRadioButton::toggled, this, [this]() {
        ui.intervalSpinBox->setEnabled(true);
    });
    connect(ui.singleRbtn, &QRadioButton::toggled, this, [this]() {
        ui.intervalSpinBox->setEnabled(false);
    });
    connect(ui.startBtn, &QPushButton::clicked, this, &fileModifier::startBtnClicked);
    connect(ui.stopBtn, &QPushButton::clicked, this, [this]() {
        if (timer->isActive()) {
            timer->stop();
            if (!proc->isWorking()){//если кнопка была нажата между запусками, обработчик не сможет дать сигнал об остановке и ui останется заблоктрованным
                setUiEnabled(true);
            }
        }
        proc->stop();
    });
    connect(proc, &fileProcessor::progress, this, [this](qint64 curr, qint64 total) {
        ui.progressBar->setValue(curr * 100 / total);
    });
    connect(proc, &fileProcessor::error, this, [this](QString info) {
        QMessageBox::warning(this, "Ошибка", info);
        setUiEnabled(true);
    });
    connect(proc, &fileProcessor::finished, this, [this](startMode StartMode) {
        if (StartMode == startMode::single) {
            QMessageBox::information(this, "Информация", "Обработка успешно завершена");
            setUiEnabled(true);
        }
        qDebug() << "успешно завершили обработку пачки";
    });
    connect(proc, &fileProcessor::stopped, this, [this]() {\
        qDebug() << "поймали сигнал остановки";
        setUiEnabled(true);
    });
}

void fileModifier::selectDirBtnClicked(QLineEdit* targetField) {
    QString path = QFileDialog::getExistingDirectory(this, "Обзор", targetField->text(), QFileDialog::ShowDirsOnly);
    if (!path.isEmpty()) targetField->setText(path);
}

void fileModifier::startBtnClicked() {
    if (!settingsIsCorrect()) return;

    setUiEnabled(false);
    setProcSettings();

    startMode StartMode = ui.singleRbtn->isChecked() ? startMode::single : startMode::regular;
    if (StartMode == startMode::regular) {
        timer->setInterval(ui.intervalSpinBox->value() * 1000);
        timer->start();
        return;
    }
    emitRunSignal(startMode::single);
}

bool fileModifier::settingsIsCorrect() {
    QString pathFrom = ui.dirFromField->text();
    QString pathTo = ui.dirToField->text();
    QString strKey = ui.xorField->text().remove("-");

    bool hasErr = false;

    if (pathFrom.isEmpty() || !QDir(pathFrom).exists()) {
        highlightFieldErr(ui.dirFromField);
        hasErr = true;
    }
    if (pathTo.isEmpty() || !QDir(pathTo).exists()) {
        highlightFieldErr(ui.dirToField);
        hasErr = true;
    }

    if (strKey.length() != 16) {
        ui.keyInfoLabel->setText("Некорректный ключ.");
        hasErr = true;
    }
    else ui.keyInfoLabel->clear();

    if (ui.maskField->text().isEmpty()) {
        ui.maskInfoLabel->setText("Поле не может быть пустым.");
        hasErr = true;
    }
    else ui.maskInfoLabel->clear();

    if (hasErr) return false;

    if (!ui.createNewRbtn->isChecked() && !ui.overwriteRbtn->isChecked() || !ui.regularRbtn->isChecked() && !ui.singleRbtn->isChecked()) {
        QMessageBox::information(this, "Форма не заполнена", "Выберите режимы");
        return false;
    }

    if (!QFileInfo(pathFrom).isReadable()){
        QMessageBox::warning(this, "Ошибка", "Чтение из этой директории запрещено");
        return false;
    }

    if (!QFileInfo(pathTo).isWritable()){
        QMessageBox::warning(this, "Ошибка", "Нет прав на запись в выбранную директорию");
        return false;
    }

    return true;
}

void fileModifier::setProcSettings() {
    QString pathFrom = ui.dirFromField->text();
    QString pathTo = ui.dirToField->text();
    QString targetDir = QDir(pathTo).absolutePath();

    QString strKey = ui.xorField->text().remove("-");
    QByteArray key = QByteArray::fromHex(strKey.toLatin1());

    saveMode SaveMode = ui.createNewRbtn->isChecked() ? saveMode::create : saveMode::overwrite;
    startMode StartMode = ui.singleRbtn->isChecked() ? startMode::single : startMode::regular;
    bool deleteOrig = ui.deleteInputCheckBox->isChecked();

    proc->setDeleteMode(deleteOrig);
    proc->setSaveMode(SaveMode);
    proc->setStartMode(StartMode);
    proc->setTargetDirectory(targetDir);
    proc->setXorKey(key);
    proc->resetStopFlag();
    proc->resetReportedFlag();
}

void fileModifier::setUiEnabled(bool enabled){
    ui.startBtn->setEnabled(enabled);
    ui.stopBtn->setEnabled(!enabled);
    ui.dirSelectBtn->setEnabled(enabled);
    ui.dirSelectBtn2->setEnabled(enabled);
    ui.dirFromField->setEnabled(enabled);
    ui.dirToField->setEnabled(enabled);
    ui.singleRbtn->setEnabled(enabled);
    ui.regularRbtn->setEnabled(enabled);
    ui.createNewRbtn->setEnabled(enabled);
    ui.overwriteRbtn->setEnabled(enabled);
    ui.deleteInputCheckBox->setEnabled(enabled);
    ui.xorField->setEnabled(enabled);
    ui.maskField->setEnabled(enabled);
    ui.intervalSpinBox->setEnabled(enabled && ui.regularRbtn->isChecked());
}

void fileModifier::emitRunSignal(const startMode StartMode) {
    if (proc->isWorking()){
        qDebug() << "уже работаем. пропускаем таймер";
        return; //для режима по таймеру (если период таймера меньше чем время обработки файлов)
    }

    QString pathFrom = ui.dirFromField->text();
    QStringList files = getFiles(pathFrom, ui.maskField->text());

    if (StartMode == startMode::single && files.isEmpty()){
        QMessageBox::information(this, "Информация", "Нет файлов для обработки. Вы можете запустить автоматический режим.");
        setUiEnabled(true);
        return;
    }
    emit runProcessor(files);
}

QStringList fileModifier::getFiles(const QString& path, const QString& mask) {
    QDir dir(path);
    QStringList filters = mask.split(",", Qt::SkipEmptyParts);
    for (auto& filter: filters) filter = filter.trimmed();
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);
    QStringList fileNames = dir.entryList();
    QStringList paths;

    for (const auto& fileName : fileNames) {
        paths << dir.absoluteFilePath(fileName);
    }

    return paths;
}

void fileModifier::highlightFieldErr(QLineEdit* targetField) {
    QString fieldStyle = targetField->styleSheet();
    QString err = fieldStyle + "border: 1px solid red";
    targetField->setStyleSheet(err);
    QTimer::singleShot(2000, targetField, [targetField, fieldStyle]() {
        targetField->setStyleSheet(fieldStyle);
    });
}

fileModifier::~fileModifier(){
    proc->stop();
}
