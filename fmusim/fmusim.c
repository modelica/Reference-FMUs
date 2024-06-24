#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <limits.h>

#ifdef _WIN32
#include <Shlwapi.h>
#include <strsafe.h>
#include <direct.h>
#endif

#include <errno.h>

#include "FMI3.h"
#include "FMIZip.h"
#include "FMIModelDescription.h"
#include "FMIRecorder.h"
#include "FMIUtil.h"

#include "fmusim.h"
#include "fmusim_fmi1_cs.h"
#include "fmusim_fmi1_me.h"
#include "fmusim_fmi2_cs.h"
#include "fmusim_fmi2_me.h"
#include "fmusim_fmi3_cs.h"
#include "fmusim_fmi3_me.h"

#include "fmusim_input.h"

#include "FMIEuler.h"
#include "FMICVode.h"

#define FMI_PATH_MAX 4096

#define PROGNAME "fmusim"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


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

static FILE* s_fmiLogFile = NULL;

static void logFunctionCall(FMIInstance* instance, FMIStatus status, const char* message) {

    FILE* const stream = s_fmiLogFile ? s_fmiLogFile : stdout;
    
    fputs(message, stream);
    
    switch (status) {
    case FMIOK:
        fputs(" -> OK\n", stream);
        break;
    case FMIWarning:
        fputs(" -> Warning\n", stream);
        break;
    case FMIDiscard:
        fputs(" -> Discard\n", stream);
        break;
    case FMIError:
        fputs(" -> Error\n", stream);
        break;
    case FMIFatal:
        fputs(" -> Fatal\n", stream);
        break;
    case FMIPending:
        fputs(" -> Pending\n", stream);
        break;
    default:
        fprintf(stream, " -> Unknown status (%d)\n", status);
        break;
    }
}

void printUsage() {
    printf(
        "Usage: " PROGNAME " [OPTION]... [FMU]\n\n"
        "Simulate a Functional Mock-up Unit and write the output to result.csv.\n"
        "\n"
        "  --help                           display this help and exit\n"
        "  --version                        display the program version\n"
        "  --interface-type [me|cs]         the interface type to use\n"
        "  --tolerance [TOLERANCE]          relative tolerance\n"
        "  --start-time [VALUE]             start time\n"
        "  --stop-time [VALUE]              stop time\n"
        "  --output-interval [VALUE]        set the output interval\n"
        "  --start-value [name] [value]     set a start value\n"
        "  --output-variable [name]         record a specific variable\n"
        "  --input-file [FILE]              read input from a CSV file\n"
        "  --output-file [FILE]             write output to a CSV file\n"
        "  --log-fmi-calls                  log FMI calls\n"
        "  --fmi-log-file [FILE]            set the FMI log file\n"
        "  --solver [euler|cvode]           the solver to use\n"
        "  --early-return-allowed           allow early return\n"
        "  --event-mode-used                use event mode\n"
        "  --record-intermediate-values     record outputs in intermediate update\n"
        "  --initial-fmu-state-file [FILE]  file to read the serialized FMU state\n"
        "  --final-fmu-state-file [FILE]    file to save the serialized FMU state\n"
        "\n"
        "Example:\n"
        "\n"
        "  " PROGNAME " BouncingBall.fmu  simulate with the default settings\n"
    );
}

FMIStatus applyStartValues(FMIInstance* S, const FMISimulationSettings* settings) {

    FMIStatus status = FMIOK;

    size_t nValues = 0;
    void* values = NULL;

    bool configurationMode = false;

    for (size_t i = 0; i < settings->nStartValues; i++) {

        const FMIModelVariable* variable = settings->startVariables[i];
        const FMICausality causality = variable->causality;
        const FMIValueReference vr = variable->valueReference;
        const FMIVariableType type = variable->type;
        const char* literal = settings->startValues[i];

        if (causality == FMIStructuralParameter && type == FMIUInt64Type) {

            CALL(FMIParseValues(FMIMajorVersion3, type, literal, &nValues, &values));

            if (!configurationMode) {
                CALL(FMI3EnterConfigurationMode(S));
                configurationMode = true;
            }

            CALL(FMI3SetUInt64(S, &vr, 1, (fmi3UInt64*)values, nValues));

            free(values);
            values = NULL;
        }
    }

    if (configurationMode) {
        CALL(FMI3ExitConfigurationMode(S));
    }

    for (size_t i = 0; i < settings->nStartValues; i++) {

        const FMIModelVariable* variable = settings->startVariables[i];
        const FMICausality causality = variable->causality;
        const FMIValueReference vr = variable->valueReference;
        const FMIVariableType type = variable->type;
        const char* literal = settings->startValues[i];

        if (causality == FMIStructuralParameter) {
            continue;
        }

        CALL(FMIParseValues(S->fmiMajorVersion, type, literal, &nValues, &values));


        if (variable->type == FMIBinaryType) {

            const size_t size = strlen(literal) / 2;
            CALL(FMI3SetBinary(S, &vr, 1, &size, values, 1));
        
        } else {

            if (S->fmiMajorVersion == FMIMajorVersion1) {
                CALL(FMI1SetValues(S, type, &vr, 1, values));
            } else if (S->fmiMajorVersion == FMIMajorVersion2) {
                CALL(FMI2SetValues(S, type, &vr, 1, values));
            } else if (S->fmiMajorVersion == FMIMajorVersion3) {
                CALL(FMI3SetValues(S, type, &vr, 1, values, nValues));
            }
        }

        free(values);
        values = NULL;
    }

TERMINATE:
    if (values) {
        free(values);
    }

    return status;
}

int main(int argc, const char* argv[]) {

    bool logFMICalls = false;

    FMIInterfaceType interfaceType = -1;

    const char* inputFile = NULL;
    const char* outputFile = NULL;
    const char* fmiLogFile = NULL;
    const char* initialFMUStateFile = NULL;
    const char* finalFMUStateFile = NULL;

    const char* startTimeLiteral = NULL;
    const char* stopTimeLiteral = NULL;
    
    double outputInterval = 0;

    size_t nStartValues = 0;
    const char** startNames = NULL;
    const char** startValues = NULL;

    size_t nOutputVariableNames = 0;
    const char** outputVariableNames = NULL;

    const char* solver = "euler";

    FMIModelDescription* modelDescription = NULL;
    FMIInstance* S = NULL;
    FMUStaticInput* input = NULL;
    FMIRecorder* result = NULL;
    const char* unzipdir = NULL;
    FMIStatus status = FMIFatal;
    bool earlyReturnAllowed = false;
    bool eventModeUsed = false;
    bool recordIntermediateValues = false;
    double tolerance = 0;

    if (argc < 2) {
        printf("Missing argument [FMU].\n\n");
        printUsage();
        status = FMIError;
        goto TERMINATE;
    } else if (argc == 2 && !strcmp(argv[1], "--help")) {
        printUsage();
        goto TERMINATE;
    } else if (argc == 2 && !strcmp(argv[1], "--version")) {
        printf(PROGNAME " "
#ifdef FMUSIM_VERSION
            FMUSIM_VERSION
#endif
            " (" FMI_PLATFORM_TUPLE ")\n");
        goto TERMINATE;
    }

    for (int i = 1; i < argc - 1; i++) {

        const char* v = argv[i];

        if (!strcmp(v, "--log-fmi-calls")) {
            logFMICalls = true;
        } else if (!strcmp(v, "--interface-type")) {
            if (!strcmp(argv[i + 1], "cs")) {
                interfaceType = FMICoSimulation;
            } else if (!strcmp(argv[i + 1], "me")) {
                interfaceType = FMIModelExchange;
            } else {
                printf(PROGNAME ": unrecognized interface type '%s'\n", argv[i + 1]);
                printf("Try '" PROGNAME " --help' for more information.\n");
                status = FMIError;
                goto TERMINATE;
            }
            i++;
        } else if (!strcmp(v, "--start-value")) {
            CALL(FMIRealloc((void**)&startNames, sizeof(char*) * (nStartValues + 1)));
            CALL(FMIRealloc((void**)&startValues, sizeof(char*) * (nStartValues + 1)));
            startNames[nStartValues] = argv[++i];
            startValues[nStartValues] = argv[++i];
            nStartValues++;
        } else if (!strcmp(v, "--output-variable")) {
            CALL(FMIRealloc((void**)&outputVariableNames, sizeof(char*) * (nOutputVariableNames + 1)));
            outputVariableNames[nOutputVariableNames] = argv[++i];
            nOutputVariableNames++;
        } else if (!strcmp(v, "--input-file")) {
            inputFile = argv[++i];
        } else if (!strcmp(v, "--output-file")) {
            outputFile = argv[++i];
        } else if (!strcmp(v, "--fmi-log-file")) {
            fmiLogFile = argv[++i];
        } else if (!strcmp(v, "--tolerance")) {
            char* error;
            tolerance = strtod(argv[++i], &error);
        } else if (!strcmp(v, "--start-time")) {
            startTimeLiteral = argv[++i];
        } else if (!strcmp(v, "--stop-time")) {
            stopTimeLiteral = argv[++i];
        } else if (!strcmp(v, "--output-interval")) {
            char* error;
            outputInterval = strtod(argv[++i], &error);
        } else if (!strcmp(v, "--solver")) {
            solver = argv[++i];
        } else if (!strcmp(v, "--early-return-allowed")) {
            earlyReturnAllowed = true;
        } else if (!strcmp(v, "--event-mode-used")) {
            eventModeUsed = true;
        } else if (!strcmp(v, "--record-intermediate-values")) {
            recordIntermediateValues = true;
        } else if (!strcmp(v, "--initial-fmu-state-file")) {
            initialFMUStateFile = argv[++i];
        } else if (!strcmp(v, "--final-fmu-state-file")) {
            finalFMUStateFile = argv[++i];
        } else {
            printf(PROGNAME ": unrecognized option '%s'\n", v);
            printf("Try '" PROGNAME " --help' for more information.\n");
            status = FMIError;
            goto TERMINATE;
        }
    }

    const char* fmuPath = argv[argc - 1];

    if (fmiLogFile) {
        s_fmiLogFile = fopen(fmiLogFile, "w");
        if (!s_fmiLogFile) {
            printf("Failed to open FMI log file %s for writing.\n", fmiLogFile);
            goto TERMINATE;
        }
    }

    unzipdir = FMICreateTemporaryDirectory();
    
    if (!unzipdir) {
        status = FMIError;
        goto TERMINATE;
    }

    char modelDescriptionPath[FMI_PATH_MAX] = "";
    
    char platformBinaryPath[FMI_PATH_MAX] = "";

    if (FMIExtractArchive(fmuPath, unzipdir)) {
        status = FMIError;
        goto TERMINATE;
    }

    strcpy(modelDescriptionPath, unzipdir);

    if (!FMIPathAppend(modelDescriptionPath, "modelDescription.xml")) {
        status = FMIError;
        goto TERMINATE;
    }

    modelDescription = FMIReadModelDescription(modelDescriptionPath);

    if (!modelDescription) {
        status = FMIError;
        goto TERMINATE;
    }

    const char* modelIdentifier = NULL;

    if (interfaceType == -1) {

        if (modelDescription->coSimulation) {
            interfaceType = FMICoSimulation;
            modelIdentifier = modelDescription->coSimulation->modelIdentifier;
        } else if (modelDescription->modelExchange) {
            interfaceType = FMIModelExchange;
            modelIdentifier = modelDescription->modelExchange->modelIdentifier;
        } else  {
            printf("No supported interface type found.\n");
            status = FMIError;
            goto TERMINATE;
        }

    } else {

        if (interfaceType == FMIModelExchange && modelDescription->modelExchange) {
            interfaceType = FMIModelExchange;
            modelIdentifier = modelDescription->modelExchange->modelIdentifier;
        } else if (interfaceType == FMICoSimulation && modelDescription->coSimulation) {
            interfaceType = FMICoSimulation;
            modelIdentifier = modelDescription->coSimulation->modelIdentifier;
        } else {
            printf("Selected interface type is not supported by the FMU.\n");
            status = FMIError;
            goto TERMINATE;
        }

    }

    void* startVariablesMemory = NULL;

    CALL(FMICalloc(&startVariablesMemory, nStartValues, sizeof(FMIModelVariable*)));

    FMIModelVariable** startVariables = startVariablesMemory;

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
            status = FMIError;
            goto TERMINATE;
        }
    }

    FMIPlatformBinaryPath(unzipdir, modelIdentifier, modelDescription->fmiMajorVersion, platformBinaryPath, FMI_PATH_MAX);

    S = FMICreateInstance("instance1", logMessage, logFMICalls ? logFunctionCall : NULL);

    if (!S) {
        printf("Failed to create FMU instance.\n");
        status = FMIError;
        goto TERMINATE;
    }

    CALL(FMILoadPlatformBinary(S, platformBinaryPath));

    size_t nOutputVariables = 0;

    void* outputVariablesMemory = NULL;
    
    CALL(FMICalloc(&outputVariablesMemory, modelDescription->nModelVariables, sizeof(FMIModelVariable*)));

    FMIModelVariable** outputVariables = outputVariablesMemory;

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        if (nOutputVariableNames) {

            for (size_t j = 0; j < nOutputVariableNames; j++) {

                if (!strcmp(variable->name, outputVariableNames[j])) {
                    outputVariables[nOutputVariables++] = variable;
                    break;
                }

            }

        } else if (variable->causality == FMIOutput) {
            outputVariables[nOutputVariables++] = variable;
        }
    }

    if (!outputFile) {
        outputFile = "result.csv";
    }

    result = FMICreateRecorder(nOutputVariables, (const FMIModelVariable**)outputVariables, outputFile);

    if (!result) {
        printf("Failed to open result file %s for writing.\n", outputFile);
        status = FMIError;
        goto TERMINATE;
    }

    char resourcePath[FMI_PATH_MAX] = "";

#ifdef _WIN32
    snprintf(resourcePath, FMI_PATH_MAX, "%s\\resources\\", unzipdir);
#else
    snprintf(resourcePath, FMI_PATH_MAX, "%s/resources/", unzipdir);
#endif
    
    if (inputFile) {
        input = FMIReadInput(modelDescription, inputFile);
        if (!input) {
            status = FMIError;
            goto TERMINATE;
        }
    }

    if (!startTimeLiteral) {
        if (modelDescription->defaultExperiment && modelDescription->defaultExperiment->startTime) {
            startTimeLiteral = modelDescription->defaultExperiment->startTime;
        } else {
            startTimeLiteral = "0";
        }
    }

    const double startTime = strtod(startTimeLiteral, NULL);

    if (!stopTimeLiteral) {
        if (modelDescription->defaultExperiment && modelDescription->defaultExperiment->stopTime) {
            stopTimeLiteral = modelDescription->defaultExperiment->stopTime;
        } else {
            stopTimeLiteral = "1";
        }
    }

    const double stopTime = strtod(stopTimeLiteral, NULL);

    if (outputInterval == 0) {
        if (modelDescription->defaultExperiment && modelDescription->defaultExperiment->stepSize) {
            outputInterval = strtod(modelDescription->defaultExperiment->stepSize, NULL);
        } else {
            outputInterval = (stopTime - startTime) / 500;
        }
    }

    FMISimulationSettings settings;

    settings.tolerance                = tolerance;
    settings.nStartValues             = nStartValues;
    settings.startVariables           = (const FMIModelVariable**)startVariables;
    settings.startValues              = startValues;
    settings.startTime                = startTime;
    settings.outputInterval           = outputInterval;
    settings.stopTime                 = stopTime;
    settings.earlyReturnAllowed       = earlyReturnAllowed;
    settings.eventModeUsed            = eventModeUsed;
    settings.recordIntermediateValues = recordIntermediateValues;
    settings.initialFMUStateFile      = initialFMUStateFile;
    settings.finalFMUStateFile        = finalFMUStateFile;

    if (!strcmp("euler", solver)) {
        settings.solverCreate = FMIEulerCreate;
        settings.solverFree   = FMIEulerFree;
        settings.solverStep   = FMIEulerStep;
        settings.solverReset  = FMIEulerReset;
    } else if (!strcmp("cvode", solver)) {
        settings.solverCreate = FMICVodeCreate;
        settings.solverFree   = FMICVodeFree;
        settings.solverStep   = FMICVodeStep;
        settings.solverReset  = FMICVodeReset;
    } else {
        printf("Unknown solver: %s.", solver);
        status = FMIError;
        goto TERMINATE;
    }

    if (modelDescription->fmiMajorVersion == FMIMajorVersion1) {

        if (interfaceType == FMICoSimulation) {

            char fmuLocation[FMI_PATH_MAX] = "";
            CALL(FMIPathToURI(unzipdir, fmuLocation, FMI_PATH_MAX));

            status = simulateFMI1CS(S, modelDescription, fmuLocation, result, input, &settings);
        } else {
            status = simulateFMI1ME(S, modelDescription, result, input, &settings);
        }

    } else if (modelDescription->fmiMajorVersion == FMIMajorVersion2) {

        char resourceURI[FMI_PATH_MAX] = "";
        CALL(FMIPathToURI(resourcePath, resourceURI, FMI_PATH_MAX));

        if (interfaceType == FMICoSimulation) {
            status = simulateFMI2CS(S, modelDescription, resourceURI, result, input, &settings);
        } else {
            status = simulateFMI2ME(S, modelDescription, resourceURI, result, input, &settings);
        }

    } else {

        if (interfaceType == FMICoSimulation) {
            status = simulateFMI3CS(S, modelDescription, resourcePath, result, input, &settings);
        } else {
            status = simulateFMI3ME(S, modelDescription, resourcePath, result, input, &settings);
        }

    }

TERMINATE:

    if (input) {
        FMIFreeInput(input);
    }

    if (result) {
        FMIFreeRecorder(result);
    }

    if (modelDescription) {
        FMIFreeModelDescription(modelDescription);
    }

    if (S) {
        FMIFreeInstance(S);
    }

    if (unzipdir) {
        FMIRemoveDirectory(unzipdir);
    }

    if (unzipdir) {
        free((void*)unzipdir);
    }

    if (s_fmiLogFile) {
        fclose(s_fmiLogFile);
        s_fmiLogFile = NULL;
    }

    free(startNames);
    free(startValues);
    free(outputVariableNames);

    return (int)status;
}
