#ifndef FILEPROCESS_H
#define FILEPROCESS_H

#include <QRunnable>
#include "fileProcessor.h"

class fileProcess: public QRunnable{

public:
    fileProcess(const QString& filePath, fileProcessor* proc, bool needToDel);
    ~fileProcess();
    void run() override;

private:
    QString file;
    fileProcessor* fileProc;
    bool needToDelete;
};

#endif // FILEPROCESS_H
