#include <QDateTime>
#include "SimulationThread.h"
#include <Windows.h>

SimulationThread::SimulationThread() {}

void SimulationThread::run() {

    continueSimulation = true;

    settings->stepFinished = stepFinished;
    settings->userData = this;

    emit progressChanged(0);

    const qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    status = FMISimulate(settings);

    const qint64 endTime = QDateTime::currentMSecsSinceEpoch();

    CPUTime = (endTime - startTime) / 1000.;
}

void SimulationThread::stop() {
    continueSimulation = false;
}

bool SimulationThread::stepFinished(const FMISimulationSettings* settings, double time) {

    SimulationThread* simulationThread = static_cast<SimulationThread*>(settings->userData);

    int progress = ((time - settings->startTime) / (settings->stopTime - settings->startTime)) * 100;

    emit simulationThread->progressChanged(progress);

    return simulationThread->continueSimulation;
}
