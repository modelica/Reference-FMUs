#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "FMU.h"
#include "fmi3Functions.h"
#include "config.h"
#include "util.h"


typedef struct {
    FMU *fmu;
    fmi3Instance instance;
    FILE* outputFile;
    fmi3Float64 intermediateUpdateTime;
} InstanceEnvironment;

// tag::IntermediateUpdateCallback[]
void cb_intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment,
                           fmi3Float64 intermediateUpdateTime,
                           fmi3Boolean clocksTicked,
                           fmi3Boolean intermediateVariableSetRequested,
                           fmi3Boolean intermediateVariableGetAllowed,
                           fmi3Boolean intermediateStepFinished,
                           fmi3Boolean canReturnEarly,
                           fmi3Boolean *earlyReturnRequested,
                           fmi3Float64 *earlyReturnTime) {

    if (!instanceEnvironment) {
        return;
    }

    *earlyReturnRequested = fmi3False;

    InstanceEnvironment* env = (InstanceEnvironment*)instanceEnvironment;

    // remember the intermediateUpdateTime
    env->intermediateUpdateTime = intermediateUpdateTime;

    fmi3Status status = fmi3OK;

    // if getting intermediate output variables is allowed
    if (intermediateVariableGetAllowed) {

        // Get the output variables at time == intermediateUpdateTime
        // fmi3Get{VariableType}();
        status = recordVariables(env->outputFile, env->fmu, env->instance, intermediateUpdateTime);

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
}
// end::IntermediateUpdateCallback[]

int main(int argc, char* argv[]) {

    puts("Running BouncingBall test... ");

    // Start and stop time
    const fmi3Float64 startTime = 0;
    const fmi3Float64 stopTime = 3;
    // Communication constant step size
    const fmi3Float64 h = 0.1;

    FMU *S = loadFMU(PLATFORM_BINARY);

    if (!S) {
        return EXIT_FAILURE;
    }

    FILE *outputFile = openOutputFile("cs_intermediate_update.csv");

    if (!outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    InstanceEnvironment instanceEnvironment = {
        .fmu                    = S,
        .instance               = NULL,
        .outputFile             = outputFile,
        .intermediateUpdateTime = startTime
    };

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
        &instanceEnvironment,   // instanceEnvironment
        cb_logMessage,          // logMessage
        cb_intermediateUpdate); // intermediateUpdate

    if (s == NULL) {
        puts("Failed to instantiate FMU.");
        return EXIT_FAILURE;
    }

    instanceEnvironment.instance = s;

    // Set all start values
    // fmi3Set{VariableType}()

    fmi3Status status = fmi3OK;

    CHECK_STATUS(S->fmi3EnterInitializationMode(s, fmi3False, 0.0, startTime, fmi3True, stopTime))
    CHECK_STATUS(S->fmi3ExitInitializationMode(s))

    fmi3Float64 time = startTime;

    while (time < stopTime) {

        fmi3Boolean eventEncountered, terminateSimulation, earlyReturn;
        fmi3Float64 lastSuccessfulTime;

        CHECK_STATUS(S->fmi3DoStep(s, time, h, fmi3False, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime))

        time += h;
    };

    fmi3Status terminateStatus;

TERMINATE:

    if (status < fmi3Fatal) {
        terminateStatus = S->fmi3Terminate(s);
    }

    if (status < fmi3Fatal && terminateStatus < fmi3Fatal) {
        S->fmi3FreeInstance(s);
    }

    puts("done.");

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
