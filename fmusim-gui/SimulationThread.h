#ifndef SIMULATIONTHREAD_H
#define SIMULATIONTHREAD_H

#include <QObject>
#include <QThread>

extern "C" {
#include "FMISimulation.h"
}

class SimulationThread : public QThread
{
    Q_OBJECT

public:
    bool logFMICalls = false;
    // const char* unzipdir = nullptr;
    FMIInterfaceType interfaceType;
    const char* modelIdentifier = nullptr;
    FMISimulationSettings settings;
    QStringList messages;
    QString inputFilename;
    // QStringList functionCalls;
    FMIStatus status = FMIOK;
    // double CPUTime = 0.0;
    SimulationThread();
    void run() override;

    static void logFunctionCall(FMIInstance* instance, FMIStatus status, const char* message);
    static void logMessage(FMIInstance* instance, FMIStatus status, const char* category, const char* message);

public slots:
    void stop();

private:
    static bool stepFinished(const FMISimulationSettings* settings, double time);
    bool continueSimulation = true;

signals:
    void progressChanged(int progress);

};

#endif // SIMULATIONTHREAD_H
