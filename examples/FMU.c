#include "FMU.h"

#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifdef _WIN32
#define LOAD_SYMBOL(f) if (!(S->##f = (f##TYPE *)GetProcAddress(S->libraryHandle, #f))) goto FAIL;
#else
#define LOAD_SYMBOL(f) if (!(S->f = (f##TYPE *)dlsym(S->libraryHandle, #f))) goto FAIL;
#endif


FMU *loadFMU(const char *filename) {

    FMU *S = calloc(1, sizeof(FMU));

    if (!S) {
        return NULL; /* Memory allocation failed */
    }

    /* Load the shared library */
#if defined(_WIN32)
    S->libraryHandle = LoadLibraryA(filename);
#elif defined(__APPLE__)
    S->libraryHandle = dlopen(filename, RTLD_LAZY);
#else
    S->libraryHandle = dlopen(filename, RTLD_LAZY);
#endif

    if (!S->libraryHandle) {
        goto FAIL;
    }

    /***************************************************
    Common Functions
    ****************************************************/

    /* Inquire version numbers and set debug logging */
    LOAD_SYMBOL(fmi3GetVersion)
    LOAD_SYMBOL(fmi3SetDebugLogging)

    /* Creation and destruction of FMU instances */
    LOAD_SYMBOL(fmi3InstantiateModelExchange)
    LOAD_SYMBOL(fmi3InstantiateCoSimulation)
    LOAD_SYMBOL(fmi3InstantiateScheduledExecution)
    LOAD_SYMBOL(fmi3FreeInstance)

    /* Enter and exit initialization mode, terminate and reset */
    LOAD_SYMBOL(fmi3EnterInitializationMode)
    LOAD_SYMBOL(fmi3ExitInitializationMode)
    LOAD_SYMBOL(fmi3EnterEventMode)
    LOAD_SYMBOL(fmi3Terminate)
    LOAD_SYMBOL(fmi3Reset)

    /* Getting and setting variables values */
    LOAD_SYMBOL(fmi3GetFloat32)
    LOAD_SYMBOL(fmi3GetFloat64)
    LOAD_SYMBOL(fmi3GetInt8)
    LOAD_SYMBOL(fmi3GetUInt8)
    LOAD_SYMBOL(fmi3GetInt16)
    LOAD_SYMBOL(fmi3GetUInt16)
    LOAD_SYMBOL(fmi3GetInt32)
    LOAD_SYMBOL(fmi3GetUInt32)
    LOAD_SYMBOL(fmi3GetInt64)
    LOAD_SYMBOL(fmi3GetUInt64)
    LOAD_SYMBOL(fmi3GetBoolean)
    LOAD_SYMBOL(fmi3GetString)
    LOAD_SYMBOL(fmi3GetBinary)
    LOAD_SYMBOL(fmi3SetFloat32)
    LOAD_SYMBOL(fmi3SetFloat64)
    LOAD_SYMBOL(fmi3SetInt8)
    LOAD_SYMBOL(fmi3SetUInt8)
    LOAD_SYMBOL(fmi3SetInt16)
    LOAD_SYMBOL(fmi3SetUInt16)
    LOAD_SYMBOL(fmi3SetInt32)
    LOAD_SYMBOL(fmi3SetUInt32)
    LOAD_SYMBOL(fmi3SetInt64)
    LOAD_SYMBOL(fmi3SetUInt64)
    LOAD_SYMBOL(fmi3SetBoolean)
    LOAD_SYMBOL(fmi3SetString)
    LOAD_SYMBOL(fmi3SetBinary)

    /* Getting Variable Dependency Information */
    LOAD_SYMBOL(fmi3GetNumberOfVariableDependencies)
    LOAD_SYMBOL(fmi3GetVariableDependencies)

    /* Getting and setting the internal FMU state */
    LOAD_SYMBOL(fmi3GetFMUState)
    LOAD_SYMBOL(fmi3SetFMUState)
    LOAD_SYMBOL(fmi3FreeFMUState)
    LOAD_SYMBOL(fmi3SerializedFMUStateSize)
    LOAD_SYMBOL(fmi3SerializeFMUState)
    LOAD_SYMBOL(fmi3DeSerializeFMUState)

    /* Getting partial derivatives */
    LOAD_SYMBOL(fmi3GetDirectionalDerivative)
    LOAD_SYMBOL(fmi3GetAdjointDerivative)

    /* Entering and exiting the Configuration or Reconfiguration Mode */
    LOAD_SYMBOL(fmi3EnterConfigurationMode)
    LOAD_SYMBOL(fmi3ExitConfigurationMode)

    /* Clock related functions */
    LOAD_SYMBOL(fmi3GetClock)
    LOAD_SYMBOL(fmi3SetClock)
    LOAD_SYMBOL(fmi3GetIntervalDecimal)
    LOAD_SYMBOL(fmi3GetIntervalFraction)
    LOAD_SYMBOL(fmi3SetIntervalDecimal)
    LOAD_SYMBOL(fmi3SetIntervalFraction)
    LOAD_SYMBOL(fmi3UpdateDiscreteStates)

    /***************************************************
    Functions for Model Exchange
    ****************************************************/

    LOAD_SYMBOL(fmi3EnterContinuousTimeMode)
    LOAD_SYMBOL(fmi3CompletedIntegratorStep)

    /* Providing independent variables and re-initialization of caching */
    LOAD_SYMBOL(fmi3SetTime)
    LOAD_SYMBOL(fmi3SetContinuousStates)

    /* Evaluation of the model equations */
    LOAD_SYMBOL(fmi3GetContinuousStateDerivatives)
    LOAD_SYMBOL(fmi3GetEventIndicators)
    LOAD_SYMBOL(fmi3GetContinuousStates)
    LOAD_SYMBOL(fmi3GetNominalsOfContinuousStates)
    LOAD_SYMBOL(fmi3GetNumberOfEventIndicators)
    LOAD_SYMBOL(fmi3GetNumberOfContinuousStates)

    /***************************************************
    Functions for Co-Simulation
    ****************************************************/

    /* Simulating the FMU */
    LOAD_SYMBOL(fmi3EnterStepMode)
    LOAD_SYMBOL(fmi3GetOutputDerivatives)
    LOAD_SYMBOL(fmi3ActivateModelPartition)
    LOAD_SYMBOL(fmi3DoStep)

    return S;

FAIL:
    if (S->libraryHandle) {

        // release the library
#if defined(_WIN32)
        FreeLibrary(S->libraryHandle);
#else
        dlclose(S->libraryHandle);
#endif
    }

    free(S);

    return NULL;
}

void freeFMU(FMU *S) {

    if (!S) {
        return;
    }

    if (S->libraryHandle) {
#if defined(_WIN32)
        FreeLibrary(S->libraryHandle);
#else
        dlclose(S->libraryHandle);
#endif
    }

    free(S);
}
