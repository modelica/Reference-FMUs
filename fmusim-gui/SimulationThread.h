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
    FMIInstance* S;
    FMIModelDescription* modelDescripton;
    const char* unzipdir;
    FMIRecorder* recorder;
    FMIStaticInput* input;
    FMISimulationSettings* settings;
    FMIStatus status;
    SimulationThread();

    void run() override;

signals:
    void progressChanged(int progress);

};

#endif // SIMULATIONTHREAD_H
