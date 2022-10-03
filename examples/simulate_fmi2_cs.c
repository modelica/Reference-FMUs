#define OUTPUT_FILE  xstr(MODEL_IDENTIFIER) "_cs_out.csv"
#define LOG_FILE     xstr(MODEL_IDENTIFIER) "_cs_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    CALL(FMI2Instantiate(S,
        resourceURI(),       // fmuResourceLocation
        fmi2CoSimulation,    // fmuType
        INSTANTIATION_TOKEN, // fmuGUID
        fmi2False,           // visible
        fmi2False            // loggingOn
    ));

    // set start values
    CALL(applyStartValues(S));

    // initialize the FMU
    CALL(FMI2SetupExperiment(S, fmi2False, 0.0, startTime, fmi2True, stopTime));
    CALL(FMI2EnterInitializationMode(S));
    CALL(FMI2ExitInitializationMode(S));

    for (uint64_t step = 0;; step++) {

        CALL(recordVariables(S, outputFile));

        // calculate the current time
        const fmi2Real time = step * h;

        CALL(applyContinuousInputs(S, false));
        CALL(applyDiscreteInputs(S));

        if (time >= stopTime) {
            break;
        }

        // call instance s1 and check status
        const FMIStatus doStepStatus = FMI2DoStep(S, time, h, fmi2True);

        if (doStepStatus == fmi2Discard) {

            fmi2Boolean terminated;
            CALL(FMI2GetBooleanStatus(S, fmi2Terminated, &terminated));

            if (terminated) {

                fmi2Real lastSuccessfulTime;
                CALL(FMI2GetRealStatus(S, fmi2LastSuccessfulTime, &lastSuccessfulTime));

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
