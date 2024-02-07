#define OUTPUT_FILE  "cs_early_return_out.csv"
#define LOG_FILE     "cs_early_return_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    // tag::EarlyReturn[]
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

    fmi3Float64 time = startTime;

    // initialize the FMU
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, stopTime));

    CALL(FMI3ExitInitializationMode(S));

    // communication step size
    const fmi3Float64 stepSize = 10 * FIXED_SOLVER_STEP;

    while (true) {

        // apply continuous and discrete inputs
        CALL(applyContinuousInputs(S, time, true));
        CALL(applyDiscreteInputs(S, time));

        // record variables
        CALL(recordVariables(S, time, outputFile));

        if (terminateSimulation || time + stepSize > stopTime) {
            break;
        }

        // returns early on events
        CALL(FMI3DoStep(S,
            time,                 // currentCommunicationPoint
            stepSize,             // communicationStepSize
            fmi3True,             // noSetFMUStatePriorToCurrentPoint
            &eventEncountered,    // eventEncountered
            &terminateSimulation, // terminate
            &earlyReturn,         // earlyReturn
            &time                 // lastSuccessfulTime
        ));
    }

TERMINATE:
    return tearDown();
    // end::EarlyReturn[]
}
