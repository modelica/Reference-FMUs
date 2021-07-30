#define OUTPUT_FILE  xstr(MODEL_IDENTIFIER) "_cs_out.csv"
#define LOG_FILE     xstr(MODEL_IDENTIFIER) "_cs_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    CALL(FMI3InstantiateCoSimulation(S,
        INSTANTIATION_TOKEN, // instantiationToken
        RESOURCE_PATH,       // resourcePath
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        fmi3False,           // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        NULL                 // intermediateUpdate
    ));

    // initialize the FMU
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, startTime, fmi3True, stopTime));

    CALL(FMI3ExitInitializationMode(S));

    for (int step = 0;; step++) {

        // calculate the current time
        const fmi3Float64 time = step * h;

        CALL(recordVariables(S, outputFile));

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
