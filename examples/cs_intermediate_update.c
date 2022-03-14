#include <math.h>

#define OUTPUT_FILE  "cs_intermediate_update_out.csv"
#define LOG_FILE     "cs_intermediate_update_log.txt"

#include "util.h"


// tag::IntermediateUpdateCallback[]
void intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment,
                           fmi3Float64 intermediateUpdateTime,
                           fmi3Boolean intermediateVariableSetRequested,
                           fmi3Boolean intermediateVariableGetAllowed,
                           fmi3Boolean intermediateStepFinished,
                           fmi3Boolean canReturnEarly,
                           fmi3Boolean *earlyReturnRequested,
                           fmi3Float64 *earlyReturnTime) {

    if (!instanceEnvironment) {
        return;
    }

    FMIInstance *S = (FMIInstance *)instanceEnvironment;

    S->time = intermediateUpdateTime;

    *earlyReturnRequested = fmi3False;
    *earlyReturnTime = 0;

    // if getting intermediate output variables is allowed
    if (intermediateVariableGetAllowed) {

        // Get the output variables at time == intermediateUpdateTime
        // fmi3Get{VariableType}();
        status = recordVariables(S, outputFile);

        // If integration step in FMU solver is finished
        if (intermediateStepFinished) {
            //Forward output variables to other FMUs or write to result files
        }
    }

    // if setting intermediate output variables is allowed
    if (intermediateVariableSetRequested) {
        // Compute intermediate input variables from output variables and
        // variables from other FMUs. Use latest available output
        // variables, possibly from get functions above.
        // inputVariables = ...

        // Set the input variables at time == intermediateUpdateTime
        // fmi3Set{VariableType}();
    }

    // Internal execution in FMU will now continue

    // TODO: handle status

    // log function call
    fprintf(logFile, "intermediateUpdate("
        "instanceEnvironment=0x%p, "
        "intermediateUpdateTime=%.16g, "
        "intermediateVariableSetRequested=%d, "
        "intermediateVariableGetAllowed=%d, "
        "intermediateStepFinished=%d, "
        "canReturnEarly=%d, "
        "earlyReturnRequested=%d, "
        "earlyReturnTime=%.16g)\n",
        instanceEnvironment,
        intermediateUpdateTime,
        intermediateVariableSetRequested,
        intermediateVariableGetAllowed,
        intermediateStepFinished,
        canReturnEarly,
        *earlyReturnRequested,
        *earlyReturnTime
    );
}
// end::IntermediateUpdateCallback[]

int main(int argc, char* argv[]) {

    CALL(setUp());

    // Instantiate the FMU
    CALL(FMI3InstantiateCoSimulation(S,
        INSTANTIATION_TOKEN, // instantiationToken
        NULL,                // resourcePath
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        fmi3False,           // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        intermediateUpdate   // intermediateUpdate
    ));

    // set start values
    CALL(applyStartValues(S));

    fmi3Float64 time = startTime;

    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, stopTime));
    CALL(FMI3ExitInitializationMode(S));

    // communication step size
    const fmi3Float64 stepSize = 10 * FIXED_SOLVER_STEP;

    while (time < stopTime) {

        fmi3Boolean eventEncountered, terminateSimulation, earlyReturn;

        CALL(FMI3DoStep(S,
            time,                 // currentCommunicationPoint
            stepSize,             // communicationStepSize
            fmi3True,             // noSetFMUStatePriorToCurrentPoint
            &eventEncountered,    // eventEncountered
            &terminateSimulation, // terminate
            &earlyReturn,         // earlyReturn
            &time                 // lastSuccessfulTime
        ));

        time += stepSize;
    };

TERMINATE:
    return tearDown();
}
