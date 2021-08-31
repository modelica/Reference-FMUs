#define OUTPUT_FILE  xstr(MODEL_IDENTIFIER) "_cs_out.csv"
#define LOG_FILE     xstr(MODEL_IDENTIFIER) "_cs_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    CALL(FMI3InstantiateCoSimulation(S,
        INSTANTIATION_TOKEN, // instantiationToken
        resourcePath(),      // resourcePath
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        fmi3False,           // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        NULL                 // intermediateUpdate
    ));

    // set start values
    CALL(applyStartValues(S));

    // initialize the FMU
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, startTime, fmi3True, stopTime));

    CALL(FMI3ExitInitializationMode(S));

    for (int step = 0;; step++) {

        CALL(recordVariables(S, outputFile));

        // calculate the current time
        const fmi3Float64 time = step * h;

        CALL(applyContinuousInputs(S, false));
        CALL(applyDiscreteInputs(S));

        if (time >= stopTime) {
            break;
        }

        // call instance s1 and check status
        CALL(FMI3DoStep(S, time, h, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime));

        if (terminateSimulation) {
            printf("The FMU requested to terminate the simulation.");
            break;
        }
    }

TERMINATE:
    return tearDown();
}
