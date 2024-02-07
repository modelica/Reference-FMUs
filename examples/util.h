#ifndef util_h
#define util_h

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")
#endif

#if FMI_VERSION == 1
#include "FMI1.h"
#elif FMI_VERSION == 2
#include "FMI2.h"
#else
#include "FMI3.h"
#endif

#include "config.h"

// "stringification" macros
#define xstr(s) str(s)
#define str(s) #s

#if FMI_VERSION == 1 || FMI_VERSION == 2

#if defined(_WIN32)
#if defined(__x86_64__) || defined(_M_X64)
#define PLATFORM_BINARY  xstr(MODEL_IDENTIFIER) "\\binaries\\win64\\" xstr(MODEL_IDENTIFIER) ".dll"
#else
#define PLATFORM_BINARY  xstr(MODEL_IDENTIFIER) "\\binaries\\win32\\" xstr(MODEL_IDENTIFIER) ".dll"
#endif
#elif defined(__APPLE__)
#define PLATFORM_BINARY  xstr(MODEL_IDENTIFIER) "/binaries/darwin64/" xstr(MODEL_IDENTIFIER) ".dylib"
#else
#define PLATFORM_BINARY  xstr(MODEL_IDENTIFIER) "/binaries/linux64/" xstr(MODEL_IDENTIFIER) ".so"
#endif

#else

#if defined(__aarch64__) || defined(_M_ARM64)
#define ARCH "aarch64"
#elif defined(__x86_64__) || defined(_M_X64)
#define ARCH "x86_64"
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define ARCH "x86"
#else
#error "Unknown architecture"
#endif

#if defined(_WIN32)
#define SYS "windows"
#define SEP "\\"
#define EXT ".dll"
#elif defined(__APPLE__)
#define SYS "darwin"
#define SEP "/"
#define EXT ".dylib"
#elif defined(__linux__)
#define SYS "linux"
#define SEP "/"
#define EXT ".so"
#else
#error "Unknown platform"
#endif

#define PLATFORM_BINARY  xstr(MODEL_IDENTIFIER) SEP "binaries" SEP ARCH "-" SYS SEP xstr(MODEL_IDENTIFIER) EXT

#endif

#ifndef min
#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#endif

#ifndef max
#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

// tag::CheckStatus[]
#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)
// end::CheckStatus[]

FILE *createOutputFile(const char *filename);

#if FMI_VERSION == 1
static const fmi1Real startTime = 0;
static const fmi1Real stopTime = DEFAULT_STOP_TIME;
static const fmi1Real h = FIXED_SOLVER_STEP;

static fmi1Boolean eventEncountered;
static fmi1Boolean earlyReturn;
static fmi1Real lastSuccessfulTime;
#elif FMI_VERSION == 2
static const fmi2Real startTime = 0;
static const fmi2Real stopTime = DEFAULT_STOP_TIME;
static const fmi2Real h = FIXED_SOLVER_STEP;

static fmi2Boolean eventEncountered;
static fmi2Boolean earlyReturn;
static fmi2Real lastSuccessfulTime;
#elif FMI_VERSION == 3
static const fmi3Float64 startTime = 0;
static const fmi3Float64 stopTime = DEFAULT_STOP_TIME;
static const fmi3Float64 h = FIXED_SOLVER_STEP;

static fmi3Boolean eventEncountered;
static fmi3Boolean terminateSimulation;
static fmi3Boolean earlyReturn;
static fmi3Float64 lastSuccessfulTime;
#endif

static FMIStatus status = FMIOK;
static FILE *outputFile = NULL;
static FMIInstance *S = NULL;
static FILE *logFile = NULL;

static const char* resourcePath() {

    static char path[4096] = "";

#if FMI_VERSION == 1

#ifdef _WIN32
    _fullpath(path, xstr(MODEL_IDENTIFIER), 4096);
#else
    realpath(xstr(MODEL_IDENTIFIER), path);
#endif

#else

#ifdef _WIN32
    _fullpath(path, xstr(MODEL_IDENTIFIER) "\\resources\\", 4096);
#else
    realpath(xstr(MODEL_IDENTIFIER) "/resources", path);
    strcat(path, "/");
#endif

#endif

    return path;
}

static const char* resourceURI() {

    static char uri[4096] = "";

    const char *path = resourcePath();

#ifdef _WIN32
    DWORD length = 4096;
    UrlCreateFromPathA(path, uri, &length, 0);
#else
    strcpy(uri, "file://");
    strcat(uri, path);
#endif

    return uri;
}

double nextInputEventTime(double time);

FMIStatus applyStartValues(FMIInstance *S);

FMIStatus applyContinuousInputs(FMIInstance *S, double time, bool afterEvent);

FMIStatus applyDiscreteInputs(FMIInstance *S, double time);

FMIStatus recordVariables(FMIInstance *S, double time, FILE *outputFile);

static void logMessage(FMIInstance *instance, FMIStatus status, const char *category, const char *message) {

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

static void logFunctionCall(FMIInstance *instance, FMIStatus status, const char *message) {

    if (!logFile) {
        return;
    }

    fputs(message, logFile);

    switch (status) {
    case FMIOK:
        fputs(" -> OK\n", logFile);
        break;
    case FMIWarning:
        fputs(" -> Warning\n", logFile);
        break;
    case FMIDiscard:
        fputs(" -> Discard\n", logFile);
        break;
    case FMIError:
        fputs(" -> Error\n", logFile);
        break;
    case FMIFatal:
        fputs(" -> Fatal\n", logFile);
        break;
    case FMIPending:
        fputs(" -> Pending\n", logFile);
        break;
    default:
        fprintf(logFile, " -> Unknown status (%d)\n", status);
        break;
    }
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

    if (S) {

        if (status < FMIError) {
            FMIStatus terminateStatus =
#if FMI_VERSION == 1
                S->interfaceType == FMICoSimulation ? FMI1TerminateSlave(S) : FMI1Terminate(S);
#elif FMI_VERSION == 2
                FMI2Terminate(S);
#elif FMI_VERSION == 3
                FMI3Terminate(S);
#endif
            if (terminateStatus > status) {
                status = terminateStatus;
            }
        }

        if (status < FMIFatal) {
#if FMI_VERSION == 1 && defined(SIMULATE_CO_SIMULATION)
            FMI1FreeSlaveInstance(S);
#elif FMI_VERSION == 1 && defined(SIMULATE_MODEL_EXCHANGE)
            FMI1FreeModelInstance(S);
#elif FMI_VERSION == 2
            FMI2FreeInstance(S);
#elif FMI_VERSION == 3
            FMI3FreeInstance(S);
#endif
        }

        FMIFreeInstance(S);
        S = NULL;
    }

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

FMIStatus applyContinuousInputs(FMIInstance *S, double time, bool afterEvent) { return FMIOK; }

FMIStatus applyDiscreteInputs(FMIInstance *S, double time) { return FMIOK; }

#endif

#endif /* util_h */
