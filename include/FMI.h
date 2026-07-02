#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#ifndef FMI_MAX_MESSAGE_LENGTH
#define FMI_MAX_MESSAGE_LENGTH 4096
#endif

#ifndef FMI_STATIC
#define FMI_STATIC
#endif

#if defined(_WIN32)
#define FMI_SYSTEM "windows"
#define FMI_PLATFORM_SYSTEM "win"
#define FMI_FILE_SEPARATOR "\\"
#define FMI_SHARED_LIBRARY_EXTENSION ".dll"
#elif defined(__APPLE__)
#define FMI_SYSTEM "darwin"
#define FMI_PLATFORM_SYSTEM "darwin"
#define FMI_FILE_SEPARATOR "/"
#define FMI_SHARED_LIBRARY_EXTENSION ".dylib"
#elif defined(__linux__)
#define FMI_SYSTEM "linux"
#define FMI_PLATFORM_SYSTEM "linux"
#define FMI_FILE_SEPARATOR "/"
#define FMI_SHARED_LIBRARY_EXTENSION ".so"
#else
#error "Unknown system"
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define FMI_ARCHITECTURE "aarch64"
#define FMI_PLATFORM_BITS "64"
#elif defined(__x86_64__) || defined(_M_X64)
#define FMI_ARCHITECTURE "x86_64"
#define FMI_PLATFORM_BITS "64"
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define FMI_ARCHITECTURE "x86"
#define FMI_PLATFORM_BITS "32"
#else
#error "Unknown architecture"
#endif

#define FMI_PLATFORM FMI_PLATFORM_SYSTEM FMI_PLATFORM_BITS
#define FMI_PLATFORM_TUPLE FMI_ARCHITECTURE "-" FMI_SYSTEM


typedef enum {
    FMIOK,
    FMIWarning,
    FMIDiscard,
    FMIError,
    FMIFatal,
    FMIPending
} FMIStatus;

typedef enum {

    // FMI 3.0 variable types
    FMIFloat32Type,
    FMIFloat64Type,
    FMIInt8Type,
    FMIUInt8Type,
    FMIInt16Type,
    FMIUInt16Type,
    FMIInt32Type,
    FMIUInt32Type,
    FMIInt64Type,
    FMIUInt64Type,
    FMIBooleanType,
    FMIStringType,
    FMIBinaryType,
    FMIClockType,

    // non-variable types used as parameters
    FMIValueReferenceType,
    FMISizeTType,

    // aliases for FMI 1.0 and 2.0 variable types
    FMIRealType         = FMIFloat64Type,
    FMIIntegerType      = FMIInt32Type,

} FMIVariableType;

typedef enum {
    FMIMajorVersion1 = 1,
    FMIMajorVersion2 = 2,
    FMIMajorVersion3 = 3
} FMIMajorVersion;

typedef enum {
    FMIModelExchange,
    FMICoSimulation,
    FMIScheduledExecution
} FMIInterfaceType;

typedef enum {

    FMIStartAndEndState         = 1 << 0,
    FMIInstantiatedState        = 1 << 1,
    FMIInitializationModeState  = 1 << 2,
    FMITerminatedState          = 1 << 3,
    FMIConfigurationModeState   = 1 << 4,
    FMIReconfigurationModeState = 1 << 5,
    FMIEventModeState           = 1 << 6,
    FMIContinuousTimeModeState  = 1 << 7,
    FMIStepModeState            = 1 << 8,
    FMIClockActivationMode      = 1 << 9

} FMIState;

typedef unsigned int FMIValueReference;

typedef struct FMIInstance_ FMIInstance;

typedef struct FMI1Functions_ FMI1Functions;

typedef struct FMI2Functions_ FMI2Functions;

typedef struct FMI3Functions_ FMI3Functions;

typedef void FMILogFunctionCall(FMIInstance *instance, FMIStatus status, const char *message);

typedef void FMILogMessage(FMIInstance *instance, FMIStatus status, const char *category, const char *message);

typedef void FMILogErrorMessage(const char* message, va_list args);

extern FMILogErrorMessage* logErrorMessage;

struct FMIInstance_ {

    FMI1Functions *fmi1Functions;
    FMI2Functions *fmi2Functions;
    FMI3Functions *fmi3Functions;

#ifdef _WIN32
    HMODULE libraryHandle;
#else
    void *libraryHandle;
#endif

    void *userData;

    FMILogMessage      *logMessage;
    FMILogFunctionCall *logFunctionCall;

    char* logMessageBuffer;
    size_t logMessageBufferSize;
    size_t logMessageBufferPosition;

    void *component;

    const char *name;

    bool logFMICalls;

    FMIState state;

    FMIStatus status;

    FMIMajorVersion fmiMajorVersion;

    FMIInterfaceType interfaceType;

};

FMI_STATIC void FMIPrintToStdErr(const char* message, va_list args);

FMI_STATIC void FMILogError(const char* message, ...);

FMI_STATIC FMIStatus FMICalloc(void** memory, size_t count, size_t size);

FMI_STATIC FMIStatus FMIRealloc(void** memory, size_t size);

FMI_STATIC void FMIFree(void** memory);

FMI_STATIC FMIInstance* FMICreateInstance(const char* instanceName, FMILogMessage* logMessage, FMILogFunctionCall* logFunctionCall);

FMI_STATIC FMIStatus FMILoadPlatformBinary(FMIInstance* instance, const char* libraryPath);

FMI_STATIC void FMIFreeInstance(FMIInstance *instance);

FMI_STATIC void FMIClearLogMessageBuffer(FMIInstance* instance);

FMI_STATIC void FMIAppendToLogMessageBuffer(FMIInstance* instance, const char* format, ...);

FMI_STATIC void FMIAppendArrayToLogMessageBuffer(FMIInstance* instance, const void* values, size_t nValues, const size_t sizes[], FMIVariableType variableType);

FMI_STATIC FMIStatus FMIPathToURI(const char *path, char *uri, const size_t uriLength);

FMI_STATIC FMIStatus FMIPlatformBinaryPath(const char *unzipdir, const char *modelIdentifier, FMIMajorVersion fmiMajorVersion, char *platformBinaryPath, size_t size);

#ifdef __cplusplus
}  /* end of extern "C" { */
#endif
