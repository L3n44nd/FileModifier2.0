#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QLineEdit>
#include <qtimer.h>
#include "ui_fileModifier.h"
#include "fileProcessor.h"

class fileModifier : public QWidget
{
    Q_OBJECT

public:
    fileModifier(QWidget *parent = nullptr);
    ~fileModifier();

signals:
    void runProcessor(const QStringList& files);

private slots:
    void startBtnClicked(); 

private:
    fileProcessor* proc;
    QTimer* timer;
    Ui::fileModifierClass ui;

    void setupUI();
    void selectDirBtnClicked(QLineEdit* targetField);
    void highlightFieldErr(QLineEdit* targetField);
    bool settingsIsCorrect();
    void setProcSettings();
    void setUiEnabled(bool enabled);
    void emitRunSignal(const startMode StartMode);
    QStringList getFiles(const QString& path, const QString& mask);
};
