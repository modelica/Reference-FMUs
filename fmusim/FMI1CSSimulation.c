#include "FMI1.h"
#include "FMI1CSSimulation.h"

#define FMI_PATH_MAX 4096

#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus FMI1CSSimulate(const FMISimulationSettings* s) {

    FMIStatus status = FMIOK;

    FMIInstance* S = s->S;

    char fmuLocation[FMI_PATH_MAX] = "";

    CALL(FMIPathToURI(s->unzipdir, fmuLocation, FMI_PATH_MAX));

    CALL(FMI1InstantiateSlave(S,
        s->modelDescription->coSimulation->modelIdentifier,  // modelIdentifier
        s->modelDescription->instantiationToken,             // fmuGUID
        fmuLocation,                                         // fmuLocation
        "application/x-fmusim",                              // mimeType
        0.0,                                                 // timeout
        s->visible,                                          // visible
        fmi1False,                                           // interactive
        s->loggingOn                                         // loggingOn
    ));

    // set start values
    CALL(FMIApplyStartValues(S, s));
    CALL(FMIApplyInput(S, s->input, s->startTime, true, true, false));

    // initialize
    CALL(FMI1InitializeSlave(S, s->startTime, fmi1False, 0));

    CALL(FMISample(S, s->startTime, s->initialRecorder));

    for (unsigned long step = 0;; step++) {
        
        const fmi1Real time = s->startTime + step * s->outputInterval;

        CALL(FMISample(S, time, s->recorder));

        CALL(FMIApplyInput(S, s->input, time, true, true, false));

        if (time >= s->stopTime) {
            break;
        }

        const FMIStatus doStepStatus = FMI1DoStep(S, time, s->outputInterval, fmi1True);

        if (doStepStatus == fmi1Discard) {

            fmi1Boolean terminated;

            CALL(FMI1GetBooleanStatus(S, fmi1DoStepStatus, &terminated));

            if (terminated) {

                fmi1Real lastSuccessfulTime;

                CALL(FMI1GetRealStatus(S, fmi1LastSuccessfulTime, &lastSuccessfulTime));

                CALL(FMISample(S, time, s->recorder));

                break;
            }

        } else {
            CALL(doStepStatus);
        }

        if (s->stepFinished && !s->stepFinished(s, time)) {
            break;
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