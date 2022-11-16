#include "fmusim_fmi2.h"
#include "fmusim_fmi2_cs.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI2CS(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMISimulationResult* result,
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

    // set start values
    //CALL(applyStartValuesFMI2(S, nStartValues, startVariables, startValues));
    CALL(FMIApplyInput(S, input, settings->startTime, true, true, false));

    // initialize
    CALL(FMI2SetupExperiment(S, fmi2False, 0.0, settings->startTime, fmi2True, settings->stopTime));
    CALL(FMI2EnterInitializationMode(S));
    CALL(FMI2ExitInitializationMode(S));

    for (unsigned long step = 0;; step++) {
        
        // calculate the current time
        const fmi2Real time = settings->startTime + step * settings->outputInterval;

        CALL(FMISample(S, time, result));

        CALL(FMIApplyInput(S, input, time, true, true, false));

        if (time >= settings->stopTime) {
            break;
        }

        // call instance s1 and check status
        const FMIStatus doStepStatus = FMI2DoStep(S, time, settings->outputInterval, fmi2True);

        if (doStepStatus == fmi2Discard) {

            fmi2Boolean terminated;
            CALL(FMI2GetBooleanStatus(S, fmi2Terminated, &terminated));

            if (terminated) {

                fmi2Real lastSuccessfulTime;
                CALL(FMI2GetRealStatus(S, fmi2LastSuccessfulTime, &lastSuccessfulTime));

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

        const FMIStatus terminateStatus = FMI2Terminate(S);

        if (terminateStatus != FMIFatal) {
            FMI2FreeInstance(S);
        }
    }

    return status;
}