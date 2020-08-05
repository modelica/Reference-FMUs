#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "fmi3Functions.h"
#include "config.h"
#include "util.h"


typedef struct {
    fmi3Instance instance;
    FILE* outputFile;
    fmi3Float64 intermediateUpdateTime;
} InstanceEnvironment;

// tag::IntermediateUpdateCallback[]
fmi3Status recordVariables(InstanceEnvironment *instanceEnvironment, fmi3Float64 time) {
    fmi3ValueReference outputsVRs[2] = { vr_h, vr_v };
    fmi3Float64 y[2];
    fmi3Status status = fmi3GetFloat64(instanceEnvironment->instance, outputsVRs, 2, y, 2);
    fprintf(instanceEnvironment->outputFile, "%g,%g,%g\n", time, y[0], y[1]);
    return status;
}

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

    if (!instanceEnvironment) {
        return;
    }

    *earlyReturnRequested = fmi3False;

    InstanceEnvironment* env = (InstanceEnvironment*)instanceEnvironment;

    // remember the intermediateUpdateTime
    env->intermediateUpdateTime = intermediateUpdateTime;

    fmi3Status status = fmi3OK;

    if (eventOccurred) {
        return; // don't record events
    }

    // if getting intermediate output variables is allowed
    if (intermediateVariableGetAllowed) {

        // Get the output variables at time == intermediateUpdateTime
        // fmi3Get{VariableType}();
        status = recordVariables(env, intermediateUpdateTime);

        // If integration step in FMU solver is finished
        if (intermediateStepFinished) {
            //Forward output variables to other FMUs or write to result files
        }
    }

    // if setting intermediate output variables is allowed
    if (intermediateVariableSetAllowed) {
        // Compute intermediate input variables from output variables and
        // variables from other FMUs. Use latest available output
        // variables, possibly from get functions above.
        // inputVariables = ...

        // Set the input variables at time == intermediateUpdateTime
        // fmi3Set{VariableType}();
    }

    // Internal execution in FMU will now continue
}
// end::IntermediateUpdateCallback[]

int main(int argc, char* argv[]) {

    puts("Running BouncingBall test... ");

    // Start and stop time
    const fmi3Float64 startTime = 0;
    const fmi3Float64 stopTime = 3;
    // Communication constant step size
    const fmi3Float64 h = 0.1;

    InstanceEnvironment instanceEnvironment = {
        .instance               = NULL,
        .outputFile             = NULL,
        .intermediateUpdateTime = startTime
    };

    instanceEnvironment.outputFile = fopen("BouncingBall_iav.csv", "w");

    if (!instanceEnvironment.outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    // write the header of the CSV
    fputs("\"time\",\"h\",\"v\"\n", instanceEnvironment.outputFile);

    // Instantiate the FMU
    fmi3Instance s = fmi3InstantiateCoSimulation(
        "instance1",               // instanceName
        INSTANTIATION_TOKEN,    // instantiationToken
        NULL,                   // resourceLocation
        fmi3False,              // visible
        fmi3False,              // loggingOn
        fmi3False,              // eventModeRequired
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

    CHECK_STATUS(fmi3EnterInitializationMode(s, fmi3False, 0.0, startTime, fmi3True, stopTime))
    CHECK_STATUS(fmi3ExitInitializationMode(s))

    fmi3Float64 time = startTime;

    while (time < stopTime) {

        fmi3Boolean terminate, earlyReturn;
        fmi3Float64 lastSuccessfulTime;

        CHECK_STATUS(fmi3DoStep(s, time, h, fmi3False, &terminate, &earlyReturn, &lastSuccessfulTime))

        time += h;
    };

    fmi3Status terminateStatus;

TERMINATE:

    if (s && status != fmi3Error && status != fmi3Fatal) {
        terminateStatus = fmi3Terminate(s);
    }

    if (s && status != fmi3Fatal && terminateStatus != fmi3Fatal) {
        fmi3FreeInstance(s);
    }

    puts("done.");

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
