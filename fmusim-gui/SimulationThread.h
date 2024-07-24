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
    FMISimulationSettings* settings = nullptr;
    FMIStatus status = FMIOK;
    double CPUTime = 0.0;
    SimulationThread();
    void run() override;

public slots:
    void stop();

private:
    static bool stepFinished(const FMISimulationSettings* settings, double time);
    bool continueSimulation = true;

signals:
    void progressChanged(int progress);

};

#endif // SIMULATIONTHREAD_H
