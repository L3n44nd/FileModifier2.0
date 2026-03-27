#include "fileProcess.h"

fileProcess::fileProcess(const QString& filePath, fileProcessor* proc, bool needToDel)
    : file(filePath), fileProc(proc), needToDelete(needToDel){
    setAutoDelete(true);
    qDebug() << "задача создана";
};

void fileProcess::run(){
    if (!fileProc->processFile(file)){
        fileProc->reportStopReason();//если возникла какая-либо ошибка прерываемся
        return;
    }
    if (needToDelete) QFile::remove(file);
    fileProc->taskCompleted();
    if (fileProc->isLastTask()) fileProc->emitFinished();
}

fileProcess::~fileProcess(){qDebug() << "задача удалена";}
