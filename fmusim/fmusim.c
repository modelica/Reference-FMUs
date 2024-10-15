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

#include "FMIZip.h"
#include "FMIModelDescription.h"
#include "FMIRecorder.h"
#include "FMIUtil.h"

#include "FMISimulation.h"

#include "FMIEuler.h"
#include "FMICVode.h"

#define FMI_PATH_MAX 4096

#define PROGNAME "fmusim"

#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

static FMIStatus logLevel = FMIOK;

static FILE* s_logFile = NULL;

static void logMessage(FMIInstance* instance, FMIStatus status, const char* category, const char* message) {

    FILE* const stream = s_logFile ? s_logFile : stdout;

    if (status < logLevel) {
        return;
    }

    switch (status) {
    case FMIOK:
        fputs("[OK] ", stream);
        break;
    case FMIWarning:
        fputs("[Warning] ", stream);
        break;
    case FMIDiscard:
        fputs("[Discard] ", stream);
        break;
    case FMIError:
        fputs("[Error] ", stream);
        break;
    case FMIFatal:
        fputs("[Fatal] ", stream);
        break;
    case FMIPending:
        fputs("[Pending] ", stream);
        break;
    }

    fputs(message, stream);

    fputc('\n', stream);
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
        "Simulate a Functional Mock-up Unit\n"
        "\n"
        "  --help                           display this help and exit\n"
        "  --version                        display the program version\n"
        "  --interface-type [me|cs]         the interface type to use\n"
        "  --tolerance [TOLERANCE]          relative tolerance\n"
        "  --start-time [VALUE]             start time\n"
        "  --stop-time [VALUE]              stop time\n"
        "  --set-stop-time                  set stop time explicitly\n"
        "  --output-interval [VALUE]        set the output interval\n"
        "  --start-value [name] [value]     set a start value\n"
        "  --output-variable [name]         record a specific variable\n"
        "  --input-file [FILE]              read input from a CSV file\n"
        "  --output-file [FILE]             write output to a CSV file\n"
        "  --logging-on                     enable FMU logging\n"
        "  --log-file [FILE]                set the log file\n"
        "  --log-level [ok|warning|error]   set the log level\n"
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

int main(int argc, const char* argv[]) {

    bool loggingOn = false;
    bool logFMICalls = false;

    FMIInterfaceType interfaceType = -1;

    const char* inputFile = NULL;
    const char* outputFile = NULL;
    const char* logFile = NULL;
    const char* fmiLogFile = NULL;
    const char* initialFMUStateFile = NULL;
    const char* finalFMUStateFile = NULL;

    const char* startTimeLiteral = NULL;
    const char* stopTimeLiteral = NULL;
    bool setStopTime = false;
    
    double outputInterval = 0;

    size_t nStartValues = 0;
    const char** startNames = NULL;
    const char** startValues = NULL;

    size_t nOutputVariableNames = 0;
    const char** outputVariableNames = NULL;

    const char* solver = "euler";

    FMIModelDescription* modelDescription = NULL;
    FMIInstance* S = NULL;
    FMIStaticInput* input = NULL;
    FMIRecorder* initialRecorder = NULL;
    FMIRecorder* recorder = NULL;
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

        if (!strcmp(v, "--logging-on")) {
            loggingOn = true;
        } else if (!strcmp(v, "--log-level")) {
            if (!strcmp(argv[i + 1], "ok")) {
                logLevel = FMIOK;
            } else if (!strcmp(argv[i + 1], "warning")) {
                logLevel = FMIWarning;
            } else if (!strcmp(argv[i + 1], "error")) {
                logLevel = FMIError;
            } else {
                printf(PROGNAME ": unrecognized log level '%s'\n", argv[i + 1]);
                printf("Log level must be one of 'ok', 'warning', or 'error'.\n");
                status = FMIError;
                goto TERMINATE;
            }
            i++;
        } else if (!strcmp(v, "--log-fmi-calls")) {
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
        } else if (!strcmp(v, "--log-file")) {
            logFile = argv[++i];
        } else if (!strcmp(v, "--fmi-log-file")) {
            fmiLogFile = argv[++i];
        } else if (!strcmp(v, "--tolerance")) {
            char* error;
            tolerance = strtod(argv[++i], &error);
        } else if (!strcmp(v, "--start-time")) {
            startTimeLiteral = argv[++i];
        } else if (!strcmp(v, "--stop-time")) {
            stopTimeLiteral = argv[++i];
        } else if (!strcmp(v, "--set-stop-time")) {
            setStopTime = true;
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

    if (logFile) {
        s_logFile = fopen(logFile, "w");
        if (!s_logFile) {
            printf("Failed to open log file %s for writing.\n", logFile);
            goto TERMINATE;
        }
    }

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
            if (!strcmp(name, modelDescription->modelVariables[j]->name)) {
                startVariables[i] = modelDescription->modelVariables[j];
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

    initialRecorder = FMICreateRecorder(S, modelDescription->nModelVariables, modelDescription->modelVariables);

    if (!initialRecorder) {
        printf("Failed to create initial variable recorder.\n");
        status = FMIError;
        goto TERMINATE;
    }

    size_t nOutputVariables = 0;

    void* outputVariablesMemory = NULL;

    CALL(FMICalloc(&outputVariablesMemory, modelDescription->nModelVariables, sizeof(FMIModelVariable*)));

    FMIModelVariable** outputVariables = outputVariablesMemory;

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = modelDescription->modelVariables[i];

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

    recorder = FMICreateRecorder(S, nOutputVariables, (const FMIModelVariable**)outputVariables);

    if (!recorder) {
        printf("Failed to create output variable recorder.\n");
        status = FMIError;
        goto TERMINATE;
    }
    
    if (inputFile) {
        input = FMIReadInput(modelDescription, inputFile);
        if (!input) {
            status = FMIError;
            goto TERMINATE;
        }
    }

    double startTime = 0;

    if (startTimeLiteral) {
        startTime = strtod(startTimeLiteral, NULL);
    } else if (modelDescription->defaultExperiment && modelDescription->defaultExperiment->startTime) {
        startTime = strtod(modelDescription->defaultExperiment->startTime, NULL);
    }

    double stopTime = startTime + 1;

    if (stopTimeLiteral) {
        stopTime = strtod(stopTimeLiteral, NULL);
    } else if (modelDescription->defaultExperiment && modelDescription->defaultExperiment->stopTime) {
        stopTime = strtod(modelDescription->defaultExperiment->stopTime, NULL);
    }

    if (outputInterval == 0) {
        if (modelDescription->defaultExperiment && modelDescription->defaultExperiment->stepSize) {
            outputInterval = strtod(modelDescription->defaultExperiment->stepSize, NULL);
        } else {
            outputInterval = (stopTime - startTime) / 500;
        }
    }

    FMISimulationSettings settings = { NULL };

    settings.interfaceType            = interfaceType;
    settings.tolerance                = tolerance;
    settings.nStartValues             = nStartValues;
    settings.startVariables           = (const FMIModelVariable**)startVariables;
    settings.startValues              = startValues;
    settings.startTime                = startTime;
    settings.outputInterval           = outputInterval;
    settings.stopTime                 = stopTime;
    settings.setStopTime              = setStopTime;
    settings.earlyReturnAllowed       = earlyReturnAllowed;
    settings.eventModeUsed            = eventModeUsed;
    settings.recordIntermediateValues = recordIntermediateValues;
    settings.initialFMUStateFile      = initialFMUStateFile;
    settings.finalFMUStateFile        = finalFMUStateFile;
    settings.visible                  = false;
    settings.loggingOn                = loggingOn;

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

    settings.S                = S;
    settings.modelDescription = modelDescription;
    settings.unzipdir         = unzipdir;
    settings.initialRecorder  = initialRecorder;
    settings.recorder         = recorder;
    settings.input            = input;
 
    status = FMISimulate(&settings);

    if (outputFile) {
        
        FILE* file = fopen(outputFile, "w");

        if (!file) {
            printf("Failed to open output file %s.\n", outputFile);
            status = FMIError;
            goto TERMINATE;
        }

        FMIRecorderWriteCSV(recorder, file);

        fclose(file);
    }

TERMINATE:

    FMIFreeInput(input);
    FMIFreeRecorder(initialRecorder);
    FMIFreeRecorder(recorder);
    FMIFreeModelDescription(modelDescription);
    FMIFreeInstance(S);

    if (unzipdir) {
        FMIRemoveDirectory(unzipdir);
        FMIFree((void**)&unzipdir);
    }

    if (s_logFile) {
        fclose(s_logFile);
        s_logFile = NULL;
    }

    if (s_fmiLogFile) {
        fclose(s_fmiLogFile);
        s_fmiLogFile = NULL;
    }

    FMIFree((void**)&startNames);
    FMIFree((void**)&startValues);
    FMIFree((void**)&outputVariableNames);

    return (int)status;
}
