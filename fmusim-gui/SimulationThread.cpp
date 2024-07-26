#include <QDateTime>
#include "SimulationThread.h"
#include <Windows.h>

SimulationThread::SimulationThread() {}

void SimulationThread::run() {

    messages.clear();

    const FMIModelDescription *modelDescription = settings.modelDescription;

    continueSimulation = true;

    settings.stepFinished = stepFinished;
    settings.userData = this;

    emit progressChanged(0);

    const qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    char platformBinaryPath[2048] = "";

    FMIPlatformBinaryPath(settings.unzipdir, modelIdentifier, settings.modelDescription->fmiMajorVersion, platformBinaryPath, 2048);

    FMIInstance *S = FMICreateInstance("instance1", SimulationThread::logMessage, logFMICalls ? SimulationThread::logFunctionCall : nullptr);

    if (!S) {
        messages.append("Failed to create FMU instance.");
        status = FMIError;
        return;
    }

    S->userData = this;

    FMILoadPlatformBinary(S, platformBinaryPath);

    QList<FMIModelVariable*> recordedVariables;

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        if (variable->variability == FMITunable || variable->variability == FMIContinuous || variable->variability == FMIDiscrete) {
            recordedVariables << variable;
        }

    }

    FMIStaticInput* input = nullptr;

    if (!inputFilename.isEmpty())  {
        const std::wstring  wstr  = inputFilename.toStdWString();
        input = FMIReadInput(modelDescription, (char*)wstr.c_str());
        if (!input) {
            messages.append("Failed to load " + inputFilename + ".");
            status = FMIError;
            return;
        }
    }

    FMIRecorder* recorder = FMICreateRecorder(S, recordedVariables.size(), (const FMIModelVariable**)recordedVariables.data());

    settings.S = S;
    settings.modelDescription = modelDescription;
    settings.recorder = recorder;
    settings.input = input;

    status = FMISimulate(&settings);

    const qint64 endTime = QDateTime::currentMSecsSinceEpoch();

    messages.append("Simulation took " + QString::number((endTime - startTime) / 1000.) + "  s.");
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

void SimulationThread::logFunctionCall(FMIInstance* instance, FMIStatus status, const char* message) {

    SimulationThread *simulationThread = (SimulationThread*)instance->userData;

    QString item(message);

    switch (status) {
    case FMIOK:
        item += " -> OK";
        break;
    case FMIWarning:
        item += " -> Warning";
        break;
    case FMIDiscard:
        item += " -> Discard";
        break;
    case FMIError:
        item += " -> Error";
        break;
    case FMIFatal:
        item += " -> Fatal";
        break;
    case FMIPending:
        item += " -> Pending";
        break;
    default:
        item += " -> Unknown status";
        break;
    }

    simulationThread->messages.append(item);
}


void SimulationThread::logMessage(FMIInstance* instance, FMIStatus status, const char* category, const char* message) {

    SimulationThread *simulationThread = (SimulationThread*)instance->userData;

    simulationThread->messages.append(message);
}
