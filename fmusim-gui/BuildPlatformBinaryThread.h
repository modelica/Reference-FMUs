#ifndef BUILDPLATFORMBINARYTHREAD_H
#define BUILDPLATFORMBINARYTHREAD_H

#include <QObject>
#include <QThread>

extern "C" {
#include "FMIModelDescription.h"
#include "FMIBuildDescription.h"
}

class BuildPlatformBinaryThread : public QThread
{
    Q_OBJECT

public:
    explicit BuildPlatformBinaryThread(QObject *parent = nullptr);

    QString unzipdir;
    FMIModelDescription* modelDescription;
    FMIBuildDescription* buildDescription;
    QString cmakeCommand;
    QString cmakeGenerator;
    QString buildConfiguration;
    bool compileWithWSL;
    bool removeBuilDirectory;

    void run() override;

signals:
    void newMessage(QString message);

};

#endif // BUILDPLATFORMBINARYTHREAD_H
