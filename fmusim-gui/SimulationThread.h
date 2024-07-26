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
    FMIInterfaceType interfaceType;
    const char* modelIdentifier = nullptr;
    FMISimulationSettings* settings = nullptr;
    QStringList messages;
    QString inputFilename;
    FMIStatus status = FMIOK;
    SimulationThread(QObject *parent = nullptr);
    ~SimulationThread();
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
