#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "FMU.h"
#include "fmi3Functions.h"
#include "config.h"
#include "util.h"


//////////////////////////
// Define callback

// Callback
void cb_intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment,
                       fmi3Float64 intermediateUpdateTime,
                       fmi3Boolean eventOccurred,
                       fmi3Boolean clocksTicked,
                       fmi3Boolean intermediateVariableSetAllowed,
                       fmi3Boolean intermediateVariableGetAllowed,
                       fmi3Boolean intermediateStepFinished,
                       fmi3Boolean canReturnEarly,
                       fmi3Boolean *earlyReturnRequested,
                       fmi3Float64 *earlyReturnTime) {

    // stop here
    *earlyReturnRequested = fmi3True;
    *earlyReturnTime = intermediateUpdateTime;
}

int main(int argc, char* argv[]) {

    puts("Running BouncingBall test... ");

    // Start and stop time
    const fmi3Float64 startTime = 0;
    const fmi3Float64 stopTime = 3;
    // Communication constant step size
    const fmi3Float64 h = 0.01;

    FMU *S = loadFMU(PLATFORM_BINARY);

    if (!S) {
        return EXIT_FAILURE;
    }

    FILE *outputFile = openOutputFile("cs_early_return.csv");

    if (!outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    //////////////////////////
    // Initialization sub-phase

    // Instantiate the FMU
    fmi3Instance s = S->fmi3InstantiateCoSimulation(
        "instance1",            // instanceName
        INSTANTIATION_TOKEN,    // instantiationToken
        NULL,                   // resourceLocation
        fmi3False,              // visible
        fmi3False,              // loggingOn
        fmi3False,              // eventModeUsed
        fmi3False,              // earlyReturnAllowed
        NULL,                   // requiredIntermediateVariables
        0,                      // nRequiredIntermediateVariables
        NULL,                   // instanceEnvironment
        cb_logMessage,          // logMessage
        NULL);                  // intermediateUpdate

    if (s == NULL) {
        puts("Failed to instantiate FMU.");
        return EXIT_FAILURE;
    }

    // Set all variable start values (of "ScalarVariable / <type> / start")
    // fmi3SetReal/Integer/Boolean/String(s, ...);

    fmi3Status status = fmi3OK;

    // Initialize the FMU instance
    CHECK_STATUS(S->fmi3EnterInitializationMode(s, fmi3False, 0.0, startTime, fmi3True, stopTime))
    // Set the input values at time = startTime
    // fmi3SetReal/Integer/Boolean/String(s, ...);
    CHECK_STATUS(S->fmi3ExitInitializationMode(s))

    //////////////////////////
    // Simulation sub-phase
    fmi3Float64 time = startTime; // current time
    fmi3Float64 step = h;         // non-zero step size

    while (time < stopTime) {

        if (step > 0) {
            // Continuous mode (default mode)
            fmi3Boolean eventEncountered, terminate, earlyReturn;
            fmi3Float64 lastSuccessfulTime;

            CHECK_STATUS(S->fmi3DoStep(s, time, step, fmi3False, &eventEncountered, &terminate, &earlyReturn, &lastSuccessfulTime))

            switch (status) {
                case fmi3OK:
                    if (earlyReturn) {
                        // TODO: pass reasons
                        CHECK_STATUS(S->fmi3EnterEventMode(s, fmi3False, fmi3False, NULL, 0, fmi3False));
                        step = 0;
                        //tc = instanceEnvironment.intermediateUpdateTime;
                    } else {
                        time += step;
                        step = h;
                    }
                    break;
                case fmi3Discard:
                    // TODO: handle discard
                    break;
                default:
                    //CHECK_STATUS(status)
                    break;
            };
        } else {
            fmi3Boolean discreteStatesNeedUpdate = fmi3True;
            fmi3Boolean terminateSimulation;
            fmi3Boolean nominalsOfContinuousStatesChanged;
            fmi3Boolean valuesOfContinuousStatesChanged;
            fmi3Boolean nextEventTimeDefined;
            fmi3Float64 nextEventTime;

            // Event mode
            CHECK_STATUS(S->fmi3UpdateDiscreteStates(s, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsOfContinuousStatesChanged, &valuesOfContinuousStatesChanged, &nextEventTimeDefined, &nextEventTime))

            if (!discreteStatesNeedUpdate) {
                CHECK_STATUS(S->fmi3EnterContinuousTimeMode(s))
                step = h - fmod(time, h);  // finish the step
            };
        };

        // Get outputs
        // fmi3GetReal/Integer/Boolean/String(s, ...);
        CHECK_STATUS(recordVariables(outputFile, S, s, time))

        // Set inputs
        // fmi3SetReal/Integer/Boolean/String(s, ...);
    };

    //////////////////////////
    // Shutdown sub-phase
    fmi3Status terminateStatus;

TERMINATE:

    if (status < fmi3Fatal) {
        terminateStatus = S->fmi3Terminate(s);
    }

    if (status < fmi3Fatal && terminateStatus < fmi3Fatal) {
        S->fmi3FreeInstance(s);
    }

    freeFMU(S);

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
