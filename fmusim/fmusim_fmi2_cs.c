#include "FMIUtil.h"

#include "fmusim_fmi2_cs.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI2CS(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMIRecorder* result,
    const FMUStaticInput * input,
    const FMISimulationSettings * settings) {

    FMIStatus status = FMIOK;

    CALL(FMI2Instantiate(S,
        resourceURI,                          // fmuResourceLocation
        fmi2CoSimulation,                     // fmuType
        modelDescription->instantiationToken, // fmuGUID
        fmi2False,                            // visible
        fmi2False                             // loggingOn
    ));

    if (settings->initialFMUStateFile) {
        CALL(FMIRestoreFMUStateFromFile(S, settings->initialFMUStateFile));
    }

    CALL(applyStartValues(S, settings));
    CALL(FMIApplyInput(S, input, settings->startTime, true, true, false));

    if (!settings->initialFMUStateFile) {
        CALL(FMI2SetupExperiment(S, settings->tolerance > 0, settings->tolerance, settings->startTime, fmi2False, 0));
        CALL(FMI2EnterInitializationMode(S));
        CALL(FMI2ExitInitializationMode(S));
    }

    for (unsigned long step = 0;; step++) {
        
        const fmi2Real time = settings->startTime + step * settings->outputInterval;

        CALL(FMISample(S, time, result));

        CALL(FMIApplyInput(S, input, time, true, true, false));

        if (time >= settings->stopTime) {
            break;
        }

        const FMIStatus doStepStatus = FMI2DoStep(S, time, settings->outputInterval, fmi2True);

        if (doStepStatus == fmi2Discard) {

            fmi2Boolean terminated;
            CALL(FMI2GetBooleanStatus(S, fmi2Terminated, &terminated));

            if (terminated) {

                fmi2Real lastSuccessfulTime;

                CALL(FMI2GetRealStatus(S, fmi2LastSuccessfulTime, &lastSuccessfulTime));

                CALL(FMISample(S, lastSuccessfulTime, result));

                break;
            }

        } else {
            CALL(doStepStatus);
        }

    }

    if (settings->finalFMUStateFile) {
        CALL(FMISaveFMUStateToFile(S, settings->finalFMUStateFile));
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
