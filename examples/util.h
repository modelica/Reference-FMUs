#ifndef util_h
#define util_h

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include "FMI3.h"
#include "config.h"


// "stringification" macros
#define xstr(s) str(s)
#define str(s) #s

#if defined(_WIN32)
#define PLATFORM_BINARY  xstr(MODEL_IDENTIFIER) "\\binaries\\x86_64-windows\\" xstr(MODEL_IDENTIFIER) ".dll"
#elif defined(__APPLE__)
#define PLATFORM_BINARY  xstr(MODEL_IDENTIFIER) "/binaries/x86_64-darwin/" xstr(MODEL_IDENTIFIER) ".dylib"
#else
#define PLATFORM_BINARY  xstr(MODEL_IDENTIFIER) "/binaries/x86_64-linux/" xstr(MODEL_IDENTIFIER) ".so"
#endif

#if defined(_WIN32)
#define RESOURCE_PATH  xstr(MODEL_IDENTIFIER) "\\resources"
#else
#define RESOURCE_PATH  xstr(MODEL_IDENTIFIER) "/resources"
#endif

#ifndef min
#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#endif

#ifndef max
#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

// tag::CheckStatus[]
#define CALL(f) status = f; if (status > FMIOK) goto TERMINATE;
// end::CheckStatus[]

FILE *createOutputFile(const char *filename);

static const fmi3Float64 startTime = 0;
static const fmi3Float64 stopTime = DEFAULT_STOP_TIME;
static const fmi3Float64 h = FIXED_SOLVER_STEP;

static fmi3Boolean eventEncountered;
static fmi3Boolean terminateSimulation;
static fmi3Boolean earlyReturn;
static fmi3Float64 lastSuccessfulTime;

static FMIStatus status = FMIOK;
static FILE *outputFile = NULL;
static FMIInstance *S = NULL;
static FILE *logFile = NULL;

double nextInputEventTime(double time);

FMIStatus applyStartValues(FMIInstance *S);

FMIStatus applyContinuousInputs(FMIInstance *S, bool afterEvent);

FMIStatus applyDiscreteInputs(FMIInstance *S);

FMIStatus recordVariables(FMIInstance *S, FILE *outputFile);

static void cb_logMessage(fmi3InstanceEnvironment instanceEnvironment, fmi3String instanceName, fmi3Status status, fmi3String category, fmi3String message) {

    switch (status) {
        case fmi3OK:
            printf("[OK] ");
            break;
        case fmi3Warning:
            printf("[Warning] ");
            break;
        case fmi3Discard:
            printf("[Discard] ");
            break;
        case fmi3Error:
            printf("[Error] ");
            break;
        case fmi3Fatal:
            printf("[Fatal] ");
            break;
    }

    puts(message);
}

static void logMessage(FMIInstance *instance, FMIStatus status, const char *category, const char *message) {
    puts(message);
}

static void logFunctionCall(FMIInstance *instance, FMIStatus status, const char *message, ...) {

    if (!logFile) {
        return;
    }

    va_list args;
    va_start(args, message);

    vfprintf(logFile, message, args);

    switch (status) {
    case FMIOK:
        fprintf(logFile, " -> OK\n");
        break;
    case FMIWarning:
        fprintf(logFile, " -> Warning\n");
        break;
    case FMIDiscard:
        fprintf(logFile, " -> Discard\n");
        break;
    case FMIError:
        fprintf(logFile, " -> Error\n");
        break;
    case FMIFatal:
        fprintf(logFile, " -> Fatal\n");
        break;
    case FMIPending:
        fprintf(logFile, " -> Pending\n");
        break;
    default:
        fprintf(logFile, " -> Unknown status (%d)\n", status);
        break;
    }

    va_end(args);
}

static FMIStatus setUp() {

#ifdef OUTPUT_FILE
    outputFile = createOutputFile(OUTPUT_FILE);

    if (!outputFile) {
        printf("Failed to open %s.\n", OUTPUT_FILE);
        return FMIError;
    }
#endif

#ifdef LOG_FILE
    logFile = fopen(LOG_FILE, "w");

    if (!logFile) {
        printf("Failed to open %s.\n", LOG_FILE);
        return FMIError;
    }
#endif

    S = FMICreateInstance("instance1", PLATFORM_BINARY, logMessage, logFunctionCall);

    if (!S) {
        printf("Failed to load shared library %s.\n", PLATFORM_BINARY);
        return FMIError;
    }

    return FMIOK;
}

static FMIStatus tearDown() {

    if (status < FMIError) {
        FMIStatus terminateStatus = FMI3Terminate(S);
        status = max(status, terminateStatus);
    }

    if (status < FMIFatal) {
        FMI3FreeInstance(S);
    }

    FMIFreeInstance(S);

    if (outputFile) {
        fclose(outputFile);
    }

    if (logFile) {
        fclose(logFile);
    }

    return status;
}

#ifdef NO_INPUTS

double nextInputEventTime(double time) { return INFINITY; }

FMIStatus applyStartValues(FMIInstance *S) { return FMIOK; }

FMIStatus applyContinuousInputs(FMIInstance *S, bool afterEvent) { return FMIOK; }

FMIStatus applyDiscreteInputs(FMIInstance *S) { return FMIOK; }

#endif

#endif /* util_h */
