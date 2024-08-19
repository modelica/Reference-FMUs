#include <QDateTime>
#include "SimulationThread.h"
#include <Windows.h>


SimulationThread* SimulationThread::currentSimulationThread = nullptr;

SimulationThread::SimulationThread(QObject *parent) : QThread(parent) {
    settings = (FMISimulationSettings*)calloc(1, sizeof(FMISimulationSettings));
}

SimulationThread::~SimulationThread()
{
    if (settings) {

        if (settings->input) {
            FMIFreeInput(settings->input);
        }

        if (settings->recorder) {
            FMIFreeRecorder(settings->recorder);
        }

        free(settings);
    }
}

void SimulationThread::appendMessage(const char* message, va_list args) {
    currentSimulationThread->messages << QString("Error: ") + message;
}

void SimulationThread::run() {

    currentSimulationThread = this;

    logErrorMessage = appendMessage;

    // clean up data from previous simulation
    messages.clear();

    if (settings->input) {
        FMIFreeInput(settings->input);
        settings->input = nullptr;
    }

    if (settings->recorder) {
        FMIFreeRecorder(settings->recorder);
        settings->recorder = nullptr;
    }

    const FMIModelDescription *modelDescription = settings->modelDescription;

    continueSimulation = true;

    settings->stepFinished = stepFinished;
    settings->userData = this;

    emit progressChanged(0);

    const qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    char platformBinaryPath[2048] = "";

    FMIPlatformBinaryPath(settings->unzipdir, modelIdentifier, settings->modelDescription->fmiMajorVersion, platformBinaryPath, 2048);

    FMIInstance *S = FMICreateInstance("instance1", SimulationThread::logMessage, logFMICalls ? SimulationThread::logFunctionCall : nullptr);

    if (!S) {
        messages.append("Failed to create FMU instance.");
        status = FMIError;
        return;
    }

    S->userData = this;

    if (FMILoadPlatformBinary(S, platformBinaryPath) != FMIOK) {
        messages.append("Failed to load platfrom binary " + QString(platformBinaryPath)  + ".");
        status = FMIError;
        return;
    }

    QList<FMIModelVariable*> recordedVariables;

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = modelDescription->modelVariables[i];

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

    FMIRecorder* initialRecorder = FMICreateRecorder(S, modelDescription->nModelVariables, (const FMIModelVariable**)modelDescription->modelVariables);

    FMIRecorder* recorder = FMICreateRecorder(S, recordedVariables.size(), (const FMIModelVariable**)recordedVariables.data());

    settings->S = S;
    settings->modelDescription = modelDescription;
    settings->initialRecorder = initialRecorder;
    settings->recorder = recorder;
    settings->input = input;

    status = FMISimulate(settings);

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
