#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "FMU.h"
#include "fmi3Functions.h"
#include "config.h"
#include "util.h"


void cb_intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment,
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

    // Start and stop time
    const fmi3Float64 startTime = 0;
    const fmi3Float64 stopTime = DEFAULT_STOP_TIME;
    // Communication constant step size
    const fmi3Float64 h = 10 * FIXED_SOLVER_STEP;

    FMU *S = loadFMU(PLATFORM_BINARY);
    
    if (!S) {
        return EXIT_FAILURE;
    }

    FILE *outputFile = openOutputFile("cs_early_return.csv");

    if (!outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    // Instantiate the FMU
    fmi3Instance s = S->fmi3InstantiateCoSimulation(
        "instance1",            // instanceName
        INSTANTIATION_TOKEN,    // instantiationToken
        NULL,                   // resourceLocation
        fmi3False,              // visible
        fmi3False,              // loggingOn
        fmi3False,              // eventModeUsed
        fmi3True,               // earlyReturnAllowed
        NULL,                   // requiredIntermediateVariables
        0,                      // nRequiredIntermediateVariables
        NULL,                   // instanceEnvironment
        cb_logMessage,          // logMessage
        cb_intermediateUpdate); // intermediateUpdate

    if (s == NULL) {
        puts("Failed to instantiate FMU.");
        return EXIT_FAILURE;
    }

    // Set all start values
    // fmi3Set{VariableType}()

    fmi3Status status = fmi3OK;

    // Initialize the model
    CHECK_STATUS(S->fmi3EnterInitializationMode(s, fmi3False, 0.0, startTime, fmi3True, stopTime))
    // Set the input values at time = startTime
    // fmi3Set{VariableType}()
    CHECK_STATUS(S->fmi3ExitInitializationMode(s))

    fmi3Float64 time = startTime; // current time
    fmi3Float64 step = h;       // non-zero step size

    while (time < stopTime) {

        // Set inputs
        // fmi3Set{VariableType}()

        // Get outputs with fmi3Get{VariableType}()
        CHECK_STATUS(recordVariables(outputFile, S, s, time))

        fmi3Boolean eventEncountered, terminateSimulation, earlyReturn;
        fmi3Float64 lastSuccessfulTime;

        CHECK_STATUS(S->fmi3DoStep(s, time, step, fmi3False, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime))

        if (earlyReturn) {
            time = lastSuccessfulTime;
            step = h - fmod(time, h);  // finish the step
        } else {
            time += step;
            step = h;
        }

    };

    fmi3Status terminateStatus;

TERMINATE:

    if (status != fmi3Error && status != fmi3Fatal) {
        terminateStatus = S->fmi3Terminate(s);
    }

    if (status != fmi3Fatal && terminateStatus != fmi3Fatal) {
        S->fmi3FreeInstance(s);
    }

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
