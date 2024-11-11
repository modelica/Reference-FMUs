#include "FMIUtil.h"
#include "FMI2.h"
#include "FMI2CSSimulation.h"

#define FMI_PATH_MAX 4096

#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

FMIStatus FMI2CSSimulate(const FMISimulationSettings* s) {

    FMIStatus status = FMIOK;

    fmi2Boolean terminateSimulation = fmi2False;
    fmi2Real time = s->startTime;
    fmi2Real nextRegularPoint = 0.0;
    fmi2Real nextCommunicationPoint = 0.0;
    fmi2Real nextInputEventTime = 0.0;
    fmi2Real stepSize = 0.0;
    
    const bool canHandleVariableCommunicationStepSize = s->modelDescription->coSimulation->canHandleVariableCommunicationStepSize;

    char resourcePath[FMI_PATH_MAX] = "";
    char resourceURI[FMI_PATH_MAX] = "";

#ifdef _WIN32
    snprintf(resourcePath, FMI_PATH_MAX, "%s\\resources\\", s->unzipdir);
#else
    snprintf(resourcePath, FMI_PATH_MAX, "%s/resources/", s->unzipdir);
#endif

    CALL(FMIPathToURI(resourcePath, resourceURI, FMI_PATH_MAX));

    FMIInstance* S = s->S;

    CALL(FMI2Instantiate(S,
        resourceURI,
        fmi2CoSimulation,
        s->modelDescription->instantiationToken,
        s->visible,
        s->loggingOn
    ));

    if (s->initialFMUStateFile) {
        CALL(FMIRestoreFMUStateFromFile(S, s->initialFMUStateFile));
    }

    CALL(FMIApplyStartValues(S, s));

    if (!s->initialFMUStateFile) {
        CALL(FMI2SetupExperiment(S, s->tolerance > 0, s->tolerance, time, s->setStopTime, s->stopTime));
        CALL(FMI2EnterInitializationMode(S));
        CALL(FMIApplyInput(S, s->input, s->startTime, true, true, true));
        CALL(FMI2ExitInitializationMode(S));
    }

    CALL(FMISample(S, time, s->initialRecorder));
    CALL(FMISample(S, time, s->recorder));

    size_t nSteps = 0;

    for (;;) {
        
        if (time > s->stopTime || FMIIsClose(time, s->stopTime)) {
            break;
        }

        nextRegularPoint = s->startTime + (nSteps + 1) * s->outputInterval;

        nextCommunicationPoint = nextRegularPoint;

        nextInputEventTime = FMINextInputEvent(s->input, time);

        if (canHandleVariableCommunicationStepSize &&
            nextCommunicationPoint > nextInputEventTime &&
            !FMIIsClose(nextCommunicationPoint, nextInputEventTime)) {
            
            nextCommunicationPoint = nextInputEventTime;
        }

        if (nextCommunicationPoint > s->stopTime && !FMIIsClose(nextCommunicationPoint, s->stopTime)) {
            
            if (canHandleVariableCommunicationStepSize) {
                nextCommunicationPoint = s->stopTime;
            } else {
                break;
            }
        }

        stepSize = nextCommunicationPoint - time;

        CALL(FMIApplyInput(S, s->input, time, true, true, true));

        const FMIStatus doStepStatus = FMI2DoStep(S, time, stepSize, fmi2True);

        if (doStepStatus == fmi2Discard) {

            CALL(FMI2GetRealStatus(S, fmi2LastSuccessfulTime, &time));

            CALL(FMI2GetBooleanStatus(S, fmi2Terminated, &terminateSimulation));

        } else {

            CALL(doStepStatus);
            
            time = nextCommunicationPoint;
        }

        if (FMIIsClose(time, nextRegularPoint)) {
            nSteps++;
        }

        CALL(FMISample(S, time, s->recorder));

        if (terminateSimulation) {
            goto TERMINATE;
        }

        if (s->stepFinished && !s->stepFinished(s, time)) {
            break;
        }
    }

    if (s->finalFMUStateFile) {
        CALL(FMISaveFMUStateToFile(S, s->finalFMUStateFile));
    }

TERMINATE:

    if (status < FMIError) {

        const FMIStatus terminateStatus = FMI2Terminate(S);

        if (terminateStatus > status) {
            status = terminateStatus;
        }
    }

    if (status != FMIFatal && S->component) {
        FMI2FreeInstance(S);
    }

    return status;
}
