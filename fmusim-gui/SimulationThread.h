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
    qint64 plotInterval = 1000;
    FMIStatus logLevel = FMIOK;
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
    void setPlotVariables(QList<const FMIModelVariable*> plotVariables);

private:
    qint64 startTime;
    qint64 nextPlotTime;
    QList<const FMIModelVariable*> plotVariables;
    static SimulationThread* currentSimulationThread;
    static void appendMessage(const char* message, va_list args);
    static bool stepFinished(const FMISimulationSettings* settings, double time);
    bool continueSimulation = true;

signals:
    void progressChanged(int progress);
    void plotChanged(QString);

};

#endif // SIMULATIONTHREAD_H
