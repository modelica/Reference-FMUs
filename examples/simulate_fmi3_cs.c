#define OUTPUT_FILE  xstr(MODEL_IDENTIFIER) "_cs_out.csv"
#define LOG_FILE     xstr(MODEL_IDENTIFIER) "_cs_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    // tag::CoSimulation[]
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

    CALL(applyStartValues(S));

    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, startTime, fmi3True, stopTime));

    CALL(FMI3ExitInitializationMode(S));

    for (uint64_t step = 0;; step++) {

        const fmi3Float64 time = step * h;

        CALL(recordVariables(S, time, outputFile));

        CALL(applyContinuousInputs(S, time, false));
        CALL(applyDiscreteInputs(S, time));

        if (time >= stopTime) {
            break;
        }

        CALL(FMI3DoStep(S, time, h, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime));

        if (terminateSimulation) {
            printf("The FMU requested to terminate the simulation.");
            break;
        }
    }

TERMINATE:
    return tearDown();
    // end::CoSimulation[]
}
