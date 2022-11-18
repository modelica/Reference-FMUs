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
#include "FMISimulationResult.h"

#include "fmusim.h"
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

static void logFunctionCall(FMIInstance* instance, FMIStatus status, const char* message, ...) {

    FILE* const stream = s_fmiLogFile ? s_fmiLogFile : stdout;

    va_list args;
    va_start(args, message);
    
    vfprintf(stream, message, args);
    
    va_end(args);

    switch (status) {
    case FMIOK:
        fprintf(stream, " -> OK\n");
        break;
    case FMIWarning:
        fprintf(stream, " -> Warning\n");
        break;
    case FMIDiscard:
        fprintf(stream, " -> Discard\n");
        break;
    case FMIError:
        fprintf(stream, " -> Error\n");
        break;
    case FMIFatal:
        fprintf(stream, " -> Fatal\n");
        break;
    case FMIPending:
        fprintf(stream, " -> Pending\n");
        break;
    default:
        fprintf(stream, " -> Unknown status (%d)\n", status);
        break;
    }
}

void printUsage() {
    printf(
        "Usage: " PROGNAME " [OPTION]... [FMU]\n"
        "Simulate a Functional Mock-up Unit (FMU).\n"
        "\n"
        "  --help                        display this help and exit\n"
        "  --start-time [VALUE]          set the start time\n"
        "  --stop-time [VALUE]           set the stop time\n"
        "  --output-interval [VALUE]     set the output interval\n"
        "  --start-value [name] [value]  set a start value\n"
        "  --input-file [FILE]           read input from a CSV file\n"
        "  --output-file [FILE]          write output to a CSV file\n"
        "  --log-fmi-calls               log FMI calls\n"
        "  --fmi-log-file [FILE]         set the FMI log file\n"
        "  --solver [euler|cvode]        the solver to use\n"
        "\n"
        "Example:\n"
        "\n"
        "  " PROGNAME " BouncingBall.fmu  simulate with the default settings\n"
    );
}

static int hexchr2bin(const char hex, char* out) {

    if (out == NULL)
        return 0;

    if (hex >= '0' && hex <= '9') {
        *out = hex - '0';
    } else if (hex >= 'A' && hex <= 'F') {
        *out = hex - 'A' + 10;
    } else if (hex >= 'a' && hex <= 'f') {
        *out = hex - 'a' + 10;
    } else {
        return 0;
    }

    return 1;
}

static size_t hexs2bin(const char* hex, unsigned char** out) {

    size_t len;
    char   b1;
    char   b2;
    size_t i;

    if (hex == NULL || *hex == '\0' || out == NULL)
        return 0;

    len = strlen(hex);
    if (len % 2 != 0)
        return 0;
    len /= 2;

    *out = malloc(len);
    memset(*out, 'A', len);
    for (i = 0; i < len; i++) {
        if (!hexchr2bin(hex[i * 2], &b1) || !hexchr2bin(hex[i * 2 + 1], &b2)) {
            return 0;
        }
        (*out)[i] = (b1 << 4) | b2;
    }
    return len;
}

FMIStatus applyStartValues(FMIInstance* S, const FMISimulationSettings* settings) {

    FMIStatus status = FMIOK;

    for (size_t i = 0; i < settings->nStartValues; i++) {

        const FMIModelVariable* variable = settings->startVariables[i];
        const FMIValueReference vr = variable->valueReference;
        const FMIVariableType type = variable->type;
        const char* literal = settings->startValues[i];

        if (S->fmiVersion == FMIVersion2) {
         
            if (type == FMIRealType || type == FMIDiscreteRealType) {
            
                const fmi2Real value = strtod(literal, NULL);
                // TODO: handle errors
                CALL(FMI2SetReal(S, &vr, 1, &value));

            } else if (type == FMIIntegerType) {

                const fmi2Integer value = atoi(literal);

                CALL(FMI2SetInteger(S, &vr, 1, &value));

            } else if (type == FMIBooleanType) {

                const fmi2Boolean value = atoi(literal) != 0;

                CALL(FMI2SetBoolean(S, &vr, 1, &value));

            }  if (type == FMIStringType) {

                CALL(FMI2SetString(S, &vr, 1, &literal));

            }
        } else if (S->fmiVersion == FMIVersion3) {

            if (type == FMIFloat32Type || type == FMIDiscreteFloat32Type) {

                const fmi3Float32 value = strtof(literal, NULL);
                // TODO: handle errors
                CALL(FMI3SetFloat32(S, &vr, 1, &value, 1));

            } else if (type == FMIFloat64Type || type == FMIDiscreteFloat64Type) {

                const fmi3Float64 value = strtod(literal, NULL);
                // TODO: handle errors
                CALL(FMI3SetFloat64(S, &vr, 1, &value, 1));

            } else if (type == FMIInt8Type) {

                const fmi3Int8 value = atoi(literal);

                CALL(FMI3SetInt8(S, &vr, 1, &value, 1));

            } else if (type == FMIUInt8Type) {

                const fmi3UInt8 value = atoi(literal);

                CALL(FMI3SetUInt8(S, &vr, 1, &value, 1));

            } else if (type == FMIInt16Type) {

                const fmi3Int16 value = atoi(literal);

                CALL(FMI3SetInt16(S, &vr, 1, &value, 1));

            } else if (type == FMIUInt16Type) {

                const fmi3UInt16 value = atoi(literal);

                CALL(FMI3SetUInt16(S, &vr, 1, &value, 1));

            } else if (type == FMIInt32Type) {

                const fmi3Int32 value = atoi(literal);

                CALL(FMI3SetInt32(S, &vr, 1, &value, 1));

            } else if (type == FMIUInt32Type) {

                const fmi3UInt32 value = atoi(literal);

                CALL(FMI3SetUInt32(S, &vr, 1, &value, 1));

            } else if (type == FMIInt64Type) {

                const fmi3Int64 value = atoi(literal);

                CALL(FMI3SetInt64(S, &vr, 1, &value, 1));

            } else if (type == FMIUInt64Type) {

                const fmi3UInt64 value = atoi(literal);

                CALL(FMI3SetUInt64(S, &vr, 1, &value, 1));

            } else if (type == FMIBooleanType) {

                const fmi3Boolean value = atoi(literal) != 0;

                CALL(FMI3SetBoolean(S, &vr, 1, &value, 1));

            }  if (type == FMIStringType) {

                CALL(FMI3SetString(S, &vr, 1, &literal, 1));

            } else if (type == FMIBinaryType) {

                unsigned char* value = NULL;

                const size_t size = hexs2bin(literal, &value);
                
                CALL(FMI3SetBinary(S, &vr, 1, &size, (fmi3Binary*)&value, 1));

                free(value);

            } else if (type == FMIClockType) {

                const fmi3Clock value = atoi(literal) != 0;

                CALL(FMI3SetClock(S, &vr, 1, &value, 1));

            }
        }
    }

TERMINATE:
    return status;
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("Missing argument [FMU].\n\n");
        printUsage();
        return EXIT_FAILURE;
    } else if (argc == 2 && !strcmp(argv[1], "--help")) {
        printUsage();
        return EXIT_SUCCESS;
    }

    bool logFMICalls = false;

    FMIInterfaceType interfaceType = -1;

    char* inputFile = NULL;
    char* outputFile = NULL;
    char* fmiLogFile = NULL;

    const char* startTimeLiteral = NULL;
    const char* stopTimeLiteral = NULL;
    
    double outputInterval = 0;

    size_t nStartValues = 0;
    char** startNames = NULL;
    char** startValues = NULL;

    char* solver = "euler";

    FMIInstance* S = NULL;
    FMISimulationResult* result = NULL;
    const char* unzipdir = NULL;
    FMIStatus status = FMIFatal;

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
                return EXIT_FAILURE;
            }
            i++;
        } else if (!strcmp(v, "--start-value")) {
            startNames  = realloc(startNames, sizeof(char*) * (nStartValues + 1));
            startValues = realloc(startValues, sizeof(char*) * (nStartValues + 1));
            startNames[nStartValues]  = argv[++i];
            startValues[nStartValues] = argv[++i];
            nStartValues++;
        } else if (!strcmp(v, "--input-file")) {
            inputFile = argv[++i];
        } else if (!strcmp(v, "--output-file")) {
            outputFile = argv[++i];
        } else if (!strcmp(v, "--fmi-log-file")) {
            fmiLogFile = argv[++i];
        } else if (!strcmp(v, "--start-time")) {
            startTimeLiteral = argv[++i];
        } else if (!strcmp(v, "--stop-time")) {
            stopTimeLiteral = argv[++i];
        } else if (!strcmp(v, "--output-interval")) {
            char* error;
            outputInterval = strtod(argv[++i], &error);
        } else if (!strcmp(v, "--solver")) {
            solver = argv[++i];
        } else {
            printf(PROGNAME ": unrecognized option '%s'\n", v);
            printf("Try '" PROGNAME " --help' for more information.\n");
            return EXIT_FAILURE;
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
        return EXIT_FAILURE;
    }

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
        printf("Failed to read model description.\n");
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
            return EXIT_FAILURE;
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
            return EXIT_FAILURE;
        }

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

    FMIPlatformBinaryPath(unzipdir, modelIdentifier, modelDescription->fmiVersion, platformBinaryPath, FMI_PATH_MAX);

    S = FMICreateInstance("instance1", platformBinaryPath, logMessage, logFMICalls ? logFunctionCall : NULL);

    result = FMICreateSimulationResult(modelDescription);

    char resourcePath[FMI_PATH_MAX] = "";

    snprintf(resourcePath, FMI_PATH_MAX, "%s\\resources\\", unzipdir);

    FMUStaticInput* input = NULL;
    
    if (inputFile) {
        input = FMIReadInput(modelDescription, inputFile);
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

    settings.nStartValues = nStartValues;
    settings.startVariables = startVariables;
    settings.startValues = startValues;
    settings.startTime = startTime;
    settings.outputInterval = outputInterval;
    settings.stopTime = stopTime;

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
        return FMIError;
    }

    if (modelDescription->fmiVersion == FMIVersion2) {

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
    }

    return status;
}
