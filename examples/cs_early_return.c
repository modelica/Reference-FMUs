#define OUTPUT_FILE  "cs_early_return_out.csv"
#define LOG_FILE     "cs_early_return_log.txt"

#include "util.h"


void intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment,
                           fmi3Float64 intermediateUpdateTime,
                           fmi3Boolean clocksTicked,
                           fmi3Boolean intermediateVariableSetRequested,
                           fmi3Boolean intermediateVariableGetAllowed,
                           fmi3Boolean intermediateStepFinished,
                           fmi3Boolean canReturnEarly,
                           fmi3Boolean *earlyReturnRequested,
                           fmi3Float64 *earlyReturnTime) {

    // stop here
    *earlyReturnRequested = fmi3False;
    *earlyReturnTime = intermediateUpdateTime;
}

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
        intermediateUpdate   // intermediateUpdate
    ));

    // initialize the FMU
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, startTime, fmi3True, stopTime));

    CALL(FMI3ExitInitializationMode(S));

    for (int step = 0;; step++) {

        // calculate the current time
        const fmi3Float64 time = step * h;

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
