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

    CALL(applyStartValues(S));

    CALL(FMI2SetupExperiment(S, fmi2False, 0.0, startTime, fmi2True, stopTime));
    CALL(FMI2EnterInitializationMode(S));
    CALL(FMI2ExitInitializationMode(S));

    for (uint64_t step = 0;; step++) {

        const fmi2Real time = step * h;

        CALL(recordVariables(S, time, outputFile));

        CALL(applyContinuousInputs(S, time, false));
        CALL(applyDiscreteInputs(S, time));

        if (time >= stopTime) {
            break;
        }

        const FMIStatus doStepStatus = FMI2DoStep(S, time, h, fmi2True);

        if (doStepStatus == fmi2Discard) {

            fmi2Boolean terminated;
            CALL(FMI2GetBooleanStatus(S, fmi2Terminated, &terminated));

            if (terminated) {

                fmi2Real lastSuccessfulTime;
                CALL(FMI2GetRealStatus(S, fmi2LastSuccessfulTime, &lastSuccessfulTime));

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
