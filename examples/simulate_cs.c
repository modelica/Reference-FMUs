#include "FMU.h"
#include "util.h"
#include "config.h"

int main(int argc, char* argv[]) {

    fmi3Status status = fmi3OK;

    const fmi3Float64 startTime = 0;
    const fmi3Float64 stopTime = 3;
    const fmi3Float64 h = 1e-2;

    fmi3Boolean eventEncountered;
    fmi3Boolean terminateSimulation;
    fmi3Boolean earlyReturn;
    fmi3Float64 lastSuccessfulTime;

#if defined(_WIN32)
    const char *sharedLibrary = xstr(MODEL_IDENTIFIER) "\\binaries\\x86_64-windows\\" xstr(MODEL_IDENTIFIER) ".dll";
#elif defined(__APPLE__)
    const char *sharedLibrary = xstr(MODEL_IDENTIFIER) "/binaries/x86_64-darwin/" xstr(MODEL_IDENTIFIER) ".dylib";
#else
    const char *sharedLibrary = xstr(MODEL_IDENTIFIER) "/binaries/x86_64-linux/" xstr(MODEL_IDENTIFIER) ".so";
#endif

    FILE *outputFile = openOutputFile("BouncingBall_cs.csv");

    if (!outputFile) {
        return EXIT_FAILURE;
    }

    FMU *S = loadFMU(sharedLibrary);

    if (!S) {
        return EXIT_FAILURE;
    }

    // tag::CoSimulation[]
    ////////////////////////////
    // Initialization sub-phase

    // instantiate both FMUs
    const fmi3Instance s = S->fmi3InstantiateCoSimulation(
        "s1",                // instanceName
        INSTANTIATION_TOKEN, // instantiationToken
        NULL,                // resourceLocation
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        fmi3False,           // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        NULL,                // instanceEnvironment
        cb_logMessage,       // logMessage
        NULL);               // intermediateUpdate

    if (!s) {
        return EXIT_FAILURE;
    }

    // initialize the FMUs
    CHECK_STATUS(S->fmi3EnterInitializationMode(s, fmi3False, 0.0, startTime, fmi3True, stopTime))

    CHECK_STATUS(S->fmi3ExitInitializationMode(s))

    ////////////////////////
    // Simulation sub-phase

    for (int step = 0;; step++) {

        // calculate the current time
        const fmi3Float64 time = step * h;

        CHECK_STATUS(recordVariables(outputFile, S, s, time));

        if (time >= stopTime) {
            break;
        }

        // call instance s1 and check status
        CHECK_STATUS(S->fmi3DoStep(s, time, h, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime))

        if (terminateSimulation) {
            printf("The FMU requested to terminate the simulation.");
            break;
        }
    }

TERMINATE:

    //////////////////////////
    // Shutdown sub-phase

    if (status < fmi3Error) {

        fmi3Status terminateStatus = S->fmi3Terminate(s);

        status = max(status, terminateStatus);

        if (status < fmi3Fatal) {
            S->fmi3FreeInstance(s);
        }
    }

    freeFMU(S);

    fclose(outputFile);

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
