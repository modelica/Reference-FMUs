#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <limits.h>
#include <Shlwapi.h>
#include <strsafe.h>

#include <direct.h>

#include "FMI3.h"
#include "FMIZip.h"
#include "FMIModelDescription.h"
#include "FMISimulationResult.h"

#define FMI_PATH_MAX 4096
#define PROGNAME "fmusim"


static void logMessage(FMIInstance* instance, FMIStatus status, const char* category, const char* message) {

    switch (status) {
    case FMIOK:
        printf("[OK] ");
        break;
    case FMIWarning:
        printf("[Warning] ");
        break;
    case FMIDiscard:
        printf("[Discard] ");
        break;
    case FMIError:
        printf("[Error] ");
        break;
    case FMIFatal:
        printf("[Fatal] ");
        break;
    case FMIPending:
        printf("[Pending] ");
        break;
    }

    puts(message);
}

static void logFunctionCall(FMIInstance* instance, FMIStatus status, const char* message, ...) {

    va_list args;
    va_start(args, message);

    vprintf(message, args);

    switch (status) {
    case FMIOK:
        printf(" -> OK\n");
        break;
    case FMIWarning:
        printf(" -> Warning\n");
        break;
    case FMIDiscard:
        printf(" -> Discard\n");
        break;
    case FMIError:
        printf(" -> Error\n");
        break;
    case FMIFatal:
        printf(" -> Fatal\n");
        break;
    case FMIPending:
        printf(" -> Pending\n");
        break;
    default:
        printf(" -> Unknown status (%d)\n", status);
        break;
    }

    va_end(args);
}


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

void printUsage() {
    printf(
        "Usage: " PROGNAME " [OPTION]...[FMU]\n"
        "Simulate an FMU\n"
        "\n"
        "--help             display this help and exit\n"
        "--log-fmi-calls    Log FMI calls to the console\n"
    );
}

FMIStatus simulateFMI3CS(FMIInstance* S, const char* instantiationToken, FMISimulationResult* result) {

    fmi3Float64 time = 0;
    fmi3Float64 stopTime = 1;
    fmi3Float64 h = 0.1;

    fmi3Boolean eventEncountered;
    fmi3Boolean terminateSimulation;
    fmi3Boolean earlyReturn;
    fmi3Float64 lastSuccessfulTime;

    FMIStatus status = FMIOK;

    CALL(FMI3InstantiateCoSimulation(S,
        instantiationToken,  // instantiationToken
        NULL,                // resourcePath
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        fmi3False,           // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        NULL                 // intermediateUpdate
    ));

    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, stopTime));

    CALL(FMI3ExitInitializationMode(S));

    while (time < stopTime) {

        CALL(FMISample(S, time, result));

        CALL(FMI3DoStep(S, time, h, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime));

        time += h;
    }

TERMINATE:

    if (status != FMIFatal) {

        const FMIStatus terminateStatus = FMI3Terminate(S);
        
        if (terminateStatus != FMIFatal) {
            FMI3FreeInstance(S);
        }
    }
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("Missing argument FMU.\n\n");
        printUsage();
        return EXIT_FAILURE;
    } else if (argc == 2 && !strcmp(argv[1], "--help")) {
        printUsage();
        return EXIT_SUCCESS;
    }

    bool logFMICalls = false;

    FMIInterfaceType interfaceType = -1;

    for (int i = 1; i < argc - 1; i++) {
        const char* v = argv[i];
        if (!strcmp(v, "--log-fmi-calls")) {
            logFMICalls = true;
        } else if (!strcmp(v, "--interface-type")) {
            if (!strcmp(argv[i + 1], "CS")) {
                interfaceType = FMICoSimulation;
            } else if (!strcmp(argv[i + 1], "ME")) {
                interfaceType = FMIModelExchange;
            } else {
                printf(PROGNAME ": unrecognized interface type '%s'\n", argv[i + 1]);
                printf("Try '" PROGNAME " --help' for more information.\n");
                return EXIT_FAILURE;
            }
            i++;
        } else {
            printf(PROGNAME ": unrecognized option '%s'\n", v);
            printf("Try '" PROGNAME " --help' for more information.\n");
            return EXIT_FAILURE;
        }
    }

    const char* fmuPath = argv[argc - 1];

    const char* unzipdir = FMICreateTemporaryDirectory();

    char modelDescriptionPath[FMI_PATH_MAX] = "";
    
    char platformBinaryPath[FMI_PATH_MAX] = "";

    if (FMIExtractArchive(fmuPath, unzipdir)) {
        return EXIT_FAILURE;
    }

    strcpy(modelDescriptionPath, unzipdir);

    if (!FMIPathAppend(modelDescriptionPath, "modelDescription.xml")) {
        return EXIT_FAILURE;
    }

    FMIModelDescription* modelDescription = FMIReadModelDescription(modelDescriptionPath);

    if (!modelDescription) {
        goto TERMINATE;
    }

    FMIDumpModelDescription(modelDescription, stdout);

    FMIPlatformBinaryPath(unzipdir, modelDescription->modelIdentifier, modelDescription->fmiVersion, platformBinaryPath, FMI_PATH_MAX);

    FMIInstance* S = FMICreateInstance("instance1", platformBinaryPath, logMessage, logFMICalls ? logFunctionCall : NULL);

    FMISimulationResult* result = FMICreateSimulationResult(modelDescription);

    FMIStatus status = simulateFMI3CS(S, modelDescription->instantiationToken, result);

TERMINATE:

    if (modelDescription) {
        FMIFreeModelDescription(modelDescription);
    }

    if (S) {
        FMIFreeInstance(S);
    }

    if (unzipdir) {
        FMIRemoveDirectory(unzipdir);
    }

    if (result) {
        FMIDumpResult(result, stdout);
        FMIFreeSimulationResult(result);
    }

    if (unzipdir) {
        free(unzipdir);
    }

    return status;
}
