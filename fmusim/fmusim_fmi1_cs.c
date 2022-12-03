#include "fmusim_fmi1_cs.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI1CS(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMIRecorder* result,
    const FMUStaticInput * input,
    const FMISimulationSettings * settings) {

    FMIStatus status = FMIOK;

    CALL(FMI1InstantiateSlave(S,
        modelDescription->coSimulation->modelIdentifier,  // modelIdentifier
        modelDescription->instantiationToken,             // fmuGUID
        resourceURI,                                      // fmuLocation
        "application/x-fmusim",                           // mimeType
        0.0,                                              // timeout
        fmi1False,                                        // visible
        fmi1False,                                        // interactive
        fmi1False                                         // loggingOn
    ));

    // set start values
    CALL(applyStartValues(S, settings));
    CALL(FMIApplyInput(S, input, settings->startTime, true, true, false));

    // initialize
    CALL(FMI1InitializeSlave(S, settings->startTime, fmi1True, settings->stopTime));

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

                S->time = lastSuccessfulTime;
                CALL(FMISample(S, time, result));

                break;
            }

        } else {
            CALL(doStepStatus);
        }
        
    }

TERMINATE:

    if (status != FMIFatal) {

        const FMIStatus terminateStatus = FMI1TerminateSlave(S);

        if (terminateStatus != FMIFatal) {
            FMI1FreeSlaveInstance(S);
        }
    }

    return status;
}