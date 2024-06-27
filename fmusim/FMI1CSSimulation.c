#include "FMI1.h"
#include "FMI1CSSimulation.h"

#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus FMI1CSSimulate(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    const char* fmuLocation,
    FMIRecorder* result,
    const FMIStaticInput * input,
    const FMISimulationSettings * settings) {

    FMIStatus status = FMIOK;

    CALL(FMI1InstantiateSlave(S,
        modelDescription->coSimulation->modelIdentifier,  // modelIdentifier
        modelDescription->instantiationToken,             // fmuGUID
        fmuLocation,                                      // fmuLocation
        "application/x-fmusim",                           // mimeType
        0.0,                                              // timeout
        fmi1False,                                        // visible
        fmi1False,                                        // interactive
        fmi1False                                         // loggingOn
    ));

    // set start values
    CALL(FMIApplyStartValues(S, settings));
    CALL(FMIApplyInput(S, input, settings->startTime, true, true, false));

    // initialize
    CALL(FMI1InitializeSlave(S, settings->startTime, fmi1False, 0));

    for (unsigned long step = 0;; step++) {
        
        const fmi1Real time = settings->startTime + step * settings->outputInterval;

        CALL(FMISample(S, time, result));

        CALL(FMIApplyInput(S, input, time, true, true, false));

        if (time >= settings->stopTime) {
            break;
        }

        const FMIStatus doStepStatus = FMI1DoStep(S, time, settings->outputInterval, fmi1True);

        if (doStepStatus == fmi1Discard) {

            fmi1Boolean terminated;

            CALL(FMI1GetBooleanStatus(S, fmi1DoStepStatus, &terminated));

            if (terminated) {

                fmi1Real lastSuccessfulTime;

                CALL(FMI1GetRealStatus(S, fmi1LastSuccessfulTime, &lastSuccessfulTime));

                CALL(FMISample(S, time, result));

                break;
            }

        } else {
            CALL(doStepStatus);
        }
        
    }

TERMINATE:

    if (status < FMIError) {

        const FMIStatus terminateStatus = FMI1TerminateSlave(S);

        if (terminateStatus > status) {
            status = terminateStatus;
        }
    }

    if (status != FMIFatal) {
        FMI1FreeSlaveInstance(S);
    }

    return status;
}