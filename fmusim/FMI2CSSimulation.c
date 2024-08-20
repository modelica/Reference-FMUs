#include "FMIUtil.h"
#include "FMI2.h"
#include "FMI2CSSimulation.h"


#define FMI_PATH_MAX 4096

#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

FMIStatus FMI2CSSimulate(const FMISimulationSettings* s) {

    FMIStatus status = FMIOK;

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
        CALL(FMI2SetupExperiment(S, s->tolerance > 0, s->tolerance, s->startTime, fmi2False, 0));
        CALL(FMI2EnterInitializationMode(S));
        CALL(FMIApplyInput(S, s->input, s->startTime, true, true, false));
        CALL(FMI2ExitInitializationMode(S));
    }

    CALL(FMISample(S, s->startTime, s->initialRecorder));

    for (unsigned long step = 0;; step++) {
        
        const fmi2Real time = s->startTime + step * s->outputInterval;

        CALL(FMISample(S, time, s->recorder));

        CALL(FMIApplyInput(S, s->input, time, true, true, false));

        if (time >= s->stopTime) {
            break;
        }

        const FMIStatus doStepStatus = FMI2DoStep(S, time, s->outputInterval, fmi2True);

        if (doStepStatus == fmi2Discard) {

            fmi2Boolean terminated;
            CALL(FMI2GetBooleanStatus(S, fmi2Terminated, &terminated));

            if (terminated) {

                fmi2Real lastSuccessfulTime;

                CALL(FMI2GetRealStatus(S, fmi2LastSuccessfulTime, &lastSuccessfulTime));

                CALL(FMISample(S, lastSuccessfulTime, s->recorder));

                break;
            }

        } else {
            CALL(doStepStatus);
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

    if (status != FMIFatal) {
        FMI2FreeInstance(S);
    }

    return status;
}
