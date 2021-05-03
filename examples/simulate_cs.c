#include "FMU.h"
#include "util.h"
#include "config.h"

int main(int argc, char* argv[]) {

    fmi3Status status = fmi3OK;

    // open the output file
    FILE *outputFile = fopen(xstr(MODEL_IDENTIFIER) "_cs.csv", "w");

    if (!outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    fmi3Float64 startTime, stopTime, h;

    fmi3Instance s1 = NULL;
    fmi3Instance s2 = NULL;

#if defined(_WIN32)
    const char *sharedLibrary = xstr(MODEL_IDENTIFIER) "\\binaries\\x86_64-windows\\" xstr(MODEL_IDENTIFIER) ".dll";
#elif defined(__APPLE__)
    const char *sharedLibrary = xstr(MODEL_IDENTIFIER) "/binaries/x86_64-darwin/" xstr(MODEL_IDENTIFIER) ".dylib", RTLD_LAZY);
#else
    const char *sharedLibrary = xstr(MODEL_IDENTIFIER) "/binaries/x86_64-linux/" xstr(MODEL_IDENTIFIER) ".so", RTLD_LAZY);
#endif

    FMU *S = loadFMU(sharedLibrary);

    if (!S) {
        return EXIT_FAILURE;
    }

    // tag::CoSimulation[]
    ////////////////////////////
    // Initialization sub-phase

    // instantiate both FMUs
    s1 = S->fmi3InstantiateCoSimulation(
        "instance1",         // instanceName
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

    if (!s1) {
        return EXIT_FAILURE;
    }

    s2 = S->fmi3InstantiateCoSimulation(
        "instance2",         // instanceName
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

    if (!s2) {
        return EXIT_FAILURE;
    }

    // start and stop time
    startTime = 0;
    stopTime = 10;

    // communication step size
    h = 0.01;

    // set all variable start values (of "ScalarVariable / <type> / start")
    // s1_fmi3SetReal/Integer/Boolean/String(s1, ...);
    // s2_fmi3SetReal/Integer/Boolean/String(s2, ...);

    // initialize the FMUs
    S->fmi3EnterInitializationMode(s1, fmi3False, 0.0, startTime, fmi3True, stopTime);
    S->fmi3EnterInitializationMode(s2, fmi3False, 0.0, startTime, fmi3True, stopTime);

    // set the input values at time = startTime
    // fmi3SetReal/Integer/Boolean/String(s1, ...);
    // fmi3SetReal/Integer/Boolean/String(s2, ...);

    S->fmi3ExitInitializationMode(s1);
    S->fmi3ExitInitializationMode(s2);

    ////////////////////////
    // Simulation sub-phase

    for (int step = 0;; step++) {

        // calculate the current time
        const fmi3Float64 time = step * h;

        CHECK_STATUS(recordVariables(outputFile, S, s1, time));
        CHECK_STATUS(recordVariables(outputFile, S, s2, time));

        if (time >= stopTime) {
            break;
        }

        // retrieve outputs
        // fmi3Get{VarialeType}(s1, ...);
        // fmi3Get{VarialeType}(s2, ...);

        // set inputs
        // fmi3Set{VarialeType}(s1, ...);
        // fmi3Set{VarialeType}(s2, ...);

        // call instance s1 and check status
        fmi3Boolean eventEncountered, terminateSimulation, earlyReturn;
        fmi3Float64 lastSuccessfulTime;

        CHECK_STATUS(S->fmi3DoStep(s1, time, h, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime))

        if (terminateSimulation) {
            printf("Instance s1 requested to terminate simulation.");
            break;
        }

        // call instance s2 and check status as above
        CHECK_STATUS(S->fmi3DoStep(s2, time, h, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime))

        // ...
    }

TERMINATE:

    if (status < fmi3Error) {

        fmi3Status s = S->fmi3Terminate(s1);
        
        status = max(status, s);

        if (s < fmi3Fatal) {
            S->fmi3FreeInstance(s1);
        }
    }

    if (status < fmi3Error) {

        fmi3Status s = S->fmi3Terminate(s2);

        status = max(status, s);

        if (s < fmi3Fatal) {
            S->fmi3FreeInstance(s2);
        }
    }

    fclose(outputFile);

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}