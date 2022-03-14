#define OUTPUT_FILE  xstr(MODEL_IDENTIFIER) "_cs_out.csv"
#define LOG_FILE     xstr(MODEL_IDENTIFIER) "_cs_log.txt"
#define SIMULATE_CO_SIMULATION

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    CALL(FMI1InstantiateSlave(S,
        xstr(MODEL_IDENTIFIER),          // modelIdentifier
        INSTANTIATION_TOKEN,             // fmuGUID
        resourceURI(),                   // fmuLocation
        "application/x-reference-fmus",  // mimeType
        0.0,                             // timeout
        fmi1False,                       // visible
        fmi1False,                       // interactive
        fmi1False                        // loggingOn
    ));

    // set start values
    CALL(applyStartValues(S));

    // initialize the FMU
    CALL(FMI1InitializeSlave(S, startTime, fmi1True, stopTime));

    for (int step = 0;; step++) {

        CALL(recordVariables(S, outputFile));

        // calculate the current time
        const fmi1Real time = step * h;

        CALL(applyContinuousInputs(S, false));
        CALL(applyDiscreteInputs(S));

        if (time >= stopTime) {
            break;
        }

        // call instance s1 and check status
        const FMIStatus doStepStatus = FMI1DoStep(S, time, h, fmi1True);

        if (doStepStatus == fmi1Discard) {

            fmi1Boolean terminated;
            CALL(FMI1GetBooleanStatus(S, fmi1DoStepStatus, &terminated));

            if (terminated) {

                fmi1Real lastSuccessfulTime;
                CALL(FMI1GetRealStatus(S, fmi1LastSuccessfulTime, &lastSuccessfulTime));

                S->time = lastSuccessfulTime;
                CALL(recordVariables(S, outputFile));

                break;
            }

        } else {
            CALL(doStepStatus);
        }

    }

TERMINATE:
    return tearDown();
}
