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

    CALL(applyStartValues(S));

    CALL(FMI1InitializeSlave(S, startTime, fmi1True, stopTime));

    for (uint64_t step = 0;; step++) {

        const fmi1Real time = step * h;

        CALL(recordVariables(S, time, outputFile));

        CALL(applyContinuousInputs(S, time, false));
        CALL(applyDiscreteInputs(S, time));

        if (time >= stopTime) {
            break;
        }

        const FMIStatus doStepStatus = FMI1DoStep(S, time, h, fmi1True);

        if (doStepStatus == fmi1Discard) {

            fmi1Boolean terminated;
            CALL(FMI1GetBooleanStatus(S, fmi1DoStepStatus, &terminated));

            if (terminated) {

                fmi1Real lastSuccessfulTime;
                CALL(FMI1GetRealStatus(S, fmi1LastSuccessfulTime, &lastSuccessfulTime));

                CALL(recordVariables(S, lastSuccessfulTime, outputFile));

                break;
            }

        } else {
            CALL(doStepStatus);
        }

    }

TERMINATE:
    return tearDown();
}
