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

fmi3Status recordVariables(InstanceEnvironment instanceEnvironment, fmi3Float64 time) {
    fmi3ValueReference outputsVRs[2] = { vr_h, vr_v };
    fmi3Float64 y[2];
    fmi3Status status = fmi3GetFloat64(instanceEnvironment.instance, outputsVRs, 2, y, 2);
    fprintf(instanceEnvironment.outputFile, "%g,%g,%g\n", time, y[0], y[1]);
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

    InstanceEnvironment* env = (InstanceEnvironment*)instanceEnvironment;

    // remember the intermediateUpdateTime
    env->intermediateUpdateTime = intermediateUpdateTime;

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

    InstanceEnvironment instanceEnvironment = {
        .instance               = NULL,
        .outputFile             = NULL,
        .intermediateUpdateTime = startTime
    };

    instanceEnvironment.outputFile = fopen("BouncingBall_out.csv", "w");

    if (!instanceEnvironment.outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    // Write the header of the CSV
    fputs("\"time\",\"h\",\"v\"\n", instanceEnvironment.outputFile);

    // Instantiate the FMU
    fmi3Instance s = fmi3InstantiateCoSimulation(
        "instance1",            // instanceName
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

    // Initialize the model
    CHECK_STATUS(fmi3EnterInitializationMode(s, fmi3False, 0.0, startTime, fmi3True, stopTime))
    // Set the input values at time = startTime
    // fmi3Set{VariableType}()
    CHECK_STATUS(fmi3ExitInitializationMode(s))

    fmi3Float64 tc = startTime; // current time
    fmi3Float64 step = h;       // non-zero step size

    while (tc < stopTime) {

        // Set inputs
        // fmi3Set{VariableType}()

        // Get outputs with fmi3Get{VariableType}()
        CHECK_STATUS(recordVariables(instanceEnvironment, tc))

        fmi3Boolean terminate, earlyReturn;
        fmi3Float64 lastSuccessfulTime;

        CHECK_STATUS(fmi3DoStep(s, tc, step, fmi3False, &terminate, &earlyReturn, &lastSuccessfulTime))

        if (earlyReturn) {
            tc = instanceEnvironment.intermediateUpdateTime;
            step = h - fmod(tc, h); // finish the step
        } else {
            tc += step;
            step = h;
        }

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
