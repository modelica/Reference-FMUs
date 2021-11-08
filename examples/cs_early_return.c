#define OUTPUT_FILE  "cs_early_return_out.csv"
#define LOG_FILE     "cs_early_return_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    CALL(FMI3InstantiateCoSimulation(S,
        INSTANTIATION_TOKEN, // instantiationToken
        NULL,                // resourcePath
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        fmi3True,            // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        NULL                 // intermediateUpdate
    ));

    // set start values
    CALL(applyStartValues(S));

    // initialize the FMU
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, startTime, fmi3True, stopTime));

    CALL(FMI3ExitInitializationMode(S));

    int grid_point = 0; // current point on the grid

    fmi3Float64 time = 0;

    while (!terminateSimulation) {

        CALL(recordVariables(S, outputFile));

        if (time >= stopTime) {
            break; // stop time has been reached
        }

        // calculate the distance to the next grid point
        const fmi3Float64 step_size = (grid_point + 1) * 0.1 - time;

        // do one grid_point
        CALL(FMI3DoStep(S,
            time,                 // currentCommunicationPoint
            step_size,            // communicationStepSize
            fmi3True,             // noSetFMUStatePriorToCurrentPoint
            &eventEncountered,    // eventEncountered
            &terminateSimulation, // terminate
            &earlyReturn,         // earlyReturn
            &time                 // lastSuccessfulTime
        ));

        if (!earlyReturn) {
            grid_point++; // grid point has been reached
        }
    }

TERMINATE:
    return tearDown();
}
