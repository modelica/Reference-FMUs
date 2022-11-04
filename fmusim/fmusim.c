#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <limits.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include <errno.h>

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

FMIStatus simulateFMI3CS(FMIInstance* S, const char* instantiationToken, const char* resourcePath, FMISimulationResult* result, size_t nStartValues, const FMIModelVariable* startVariables[], const char* startValues[], double startTime, double stepSize, double stopTime) {

    fmi3Float64 time = startTime;

    fmi3Boolean eventEncountered;
    fmi3Boolean terminateSimulation;
    fmi3Boolean earlyReturn;
    fmi3Float64 lastSuccessfulTime;

    FMIStatus status = FMIOK;

    CALL(FMI3InstantiateCoSimulation(S,
        instantiationToken,  // instantiationToken
        resourcePath,        // resourcePath
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        fmi3False,           // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        NULL                 // intermediateUpdate
    ));

    for (size_t i = 0; i < nStartValues; i++) {
    
        const FMIModelVariable* variable = startVariables[i];
        const FMIValueReference vr = variable->valueReference;
        const char* literal = startValues[i];

        switch (variable->type) {
        case FMIFloat64Type: {
            const fmi3Float64 value = strtod(literal, NULL);
            // TODO: handle errors
            CALL(FMI3SetFloat64(S, &vr, 1, &value, 1));
            break;
        }
        }
    }

    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, stopTime));

    CALL(FMI3ExitInitializationMode(S));

    long nSteps = 0;

    while (time <= stopTime) {

        CALL(FMISample(S, time, result));

        CALL(FMI3DoStep(S, time, stepSize, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime));

        time = (++nSteps) * stepSize;
    }

TERMINATE:

    if (status != FMIFatal) {

        const FMIStatus terminateStatus = FMI3Terminate(S);
        
        if (terminateStatus != FMIFatal) {
            FMI3FreeInstance(S);
        }
    }

    return status;
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

    char* outputFile = NULL;
    double startTime = 0;
    double stopTime = 1;
    double outputInterval = 1e-2;

    size_t nStartValues = 0;
    char** startNames = NULL;
    char** startValues = NULL;

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
        } else if (!strcmp(v, "--start-value")) {
            startNames  = realloc(startNames, nStartValues + 1);
            startValues = realloc(startValues, nStartValues + 1);
            startNames[nStartValues] = argv[++i];
            startValues[nStartValues] = argv[++i];
            nStartValues++;
        } else if (!strcmp(v, "--output-file")) {
            outputFile = argv[++i];
        } else if (!strcmp(v, "--start-time")) {
            char* error;
            startTime = strtod(argv[++i], &error);
        } else if (!strcmp(v, "--stop-time")) {
            char* error;
            stopTime = strtod(argv[++i], &error);
            if (errno == ERANGE) {
                printf("The value provided was out of range\n");
            }
        } else if (!strcmp(v, "--output-interval")) {
            char* error;
            outputInterval = strtod(argv[++i], &error);
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

    FMIModelVariable** startVariables = calloc(nStartValues, sizeof(FMIModelVariable*));

    for (size_t i = 0; i < nStartValues; i++) {

        const char* name = startNames[i];

        for (size_t j = 0; j < modelDescription->nModelVariables; j++) {
            if (!strcmp(name, modelDescription->modelVariables[j].name)) {
                startVariables[i] = &modelDescription->modelVariables[j];
                break;
            }
        }

        if (!startVariables[i]) {
            printf("Variable %s does not exist.\n", name);
            return 1;
        }
    }

    // FMIDumpModelDescription(modelDescription, stdout);

    FMIPlatformBinaryPath(unzipdir, modelDescription->modelIdentifier, modelDescription->fmiVersion, platformBinaryPath, FMI_PATH_MAX);

    FMIInstance* S = FMICreateInstance("instance1", platformBinaryPath, logMessage, logFMICalls ? logFunctionCall : NULL);

    FMISimulationResult* result = FMICreateSimulationResult(modelDescription);

    char resourcePath[FMI_PATH_MAX] = "";

    snprintf(resourcePath, FMI_PATH_MAX, "%s\\resources\\", unzipdir);

    FMIStatus status = simulateFMI3CS(S, modelDescription->instantiationToken, resourcePath, result, nStartValues, startVariables, startValues, startTime, outputInterval, stopTime);

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

        if (outputFile) {
            FILE* file = fopen(outputFile, "w");
            FMIDumpResult(result, file);
            fclose(file);
        } else {
            FMIDumpResult(result, stdout);
        }

        FMIFreeSimulationResult(result);
    }

    if (unzipdir) {
        free(unzipdir);
    }

    return status;
}
