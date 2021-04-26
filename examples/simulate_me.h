#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include "fmi3Functions.h"

#include "util.h"
#include "config.h"

// "stringification" macros
#define xstr(s) str(s)
#define str(s) #s

typedef struct {

#if defined(_WIN32)
    HMODULE libraryHandle;
#else
    void *libraryHandle
#endif

    /***************************************************
    Common Functions
    ****************************************************/

    /* Inquire version numbers and set debug logging */
    fmi3GetVersionTYPE*      fmi3GetVersion;
    fmi3SetDebugLoggingTYPE* fmi3SetDebugLogging;

    /* Creation and destruction of FMU instances */
    fmi3InstantiateModelExchangeTYPE*         fmi3InstantiateModelExchange;
    fmi3InstantiateCoSimulationTYPE*          fmi3InstantiateCoSimulation;
    fmi3InstantiateScheduledExecutionTYPE*    fmi3InstantiateScheduledExecution;
    fmi3FreeInstanceTYPE*                     fmi3FreeInstance;

    /* Enter and exit initialization mode, terminate and reset */
    fmi3EnterInitializationModeTYPE* fmi3EnterInitializationMode;
    fmi3ExitInitializationModeTYPE*  fmi3ExitInitializationMode;
    fmi3EnterEventModeTYPE*          fmi3EnterEventMode;
    fmi3TerminateTYPE*               fmi3Terminate;
    fmi3ResetTYPE*                   fmi3Reset;

    /* Getting and setting variables values */
    fmi3GetFloat32TYPE* fmi3GetFloat32;
    fmi3GetFloat64TYPE* fmi3GetFloat64;
    fmi3GetInt8TYPE*    fmi3GetInt8;
    fmi3GetUInt8TYPE*   fmi3GetUInt8;
    fmi3GetInt16TYPE*   fmi3GetInt16;
    fmi3GetUInt16TYPE*  fmi3GetUInt16;
    fmi3GetInt32TYPE*   fmi3GetInt32;
    fmi3GetUInt32TYPE*  fmi3GetUInt32;
    fmi3GetInt64TYPE*   fmi3GetInt64;
    fmi3GetUInt64TYPE*  fmi3GetUInt64;
    fmi3GetBooleanTYPE* fmi3GetBoolean;
    fmi3GetStringTYPE*  fmi3GetString;
    fmi3GetBinaryTYPE*  fmi3GetBinary;
    fmi3SetFloat32TYPE* fmi3SetFloat32;
    fmi3SetFloat64TYPE* fmi3SetFloat64;
    fmi3SetInt8TYPE*    fmi3SetInt8;
    fmi3SetUInt8TYPE*   fmi3SetUInt8;
    fmi3SetInt16TYPE*   fmi3SetInt16;
    fmi3SetUInt16TYPE*  fmi3SetUInt16;
    fmi3SetInt32TYPE*   fmi3SetInt32;
    fmi3SetUInt32TYPE*  fmi3SetUInt32;
    fmi3SetInt64TYPE*   fmi3SetInt64;
    fmi3SetUInt64TYPE*  fmi3SetUInt64;
    fmi3SetBooleanTYPE* fmi3SetBoolean;
    fmi3SetStringTYPE*  fmi3SetString;
    fmi3SetBinaryTYPE*  fmi3SetBinary;

    /* Getting Variable Dependency Information */
    fmi3GetNumberOfVariableDependenciesTYPE* fmi3GetNumberOfVariableDependencies;
    fmi3GetVariableDependenciesTYPE*         fmi3GetVariableDependencies;

    /* Getting and setting the internal FMU state */
    fmi3GetFMUStateTYPE*            fmi3GetFMUState;
    fmi3SetFMUStateTYPE*            fmi3SetFMUState;
    fmi3FreeFMUStateTYPE*           fmi3FreeFMUState;
    fmi3SerializedFMUStateSizeTYPE* fmi3SerializedFMUStateSize;
    fmi3SerializeFMUStateTYPE*      fmi3SerializeFMUState;
    fmi3DeSerializeFMUStateTYPE*    fmi3DeSerializeFMUState;

    /* Getting partial derivatives */
    fmi3GetDirectionalDerivativeTYPE* fmi3GetDirectionalDerivative;
    fmi3GetAdjointDerivativeTYPE*     fmi3GetAdjointDerivative;

    /* Entering and exiting the Configuration or Reconfiguration Mode */
    fmi3EnterConfigurationModeTYPE* fmi3EnterConfigurationMode;
    fmi3ExitConfigurationModeTYPE*  fmi3ExitConfigurationMode;

    /* Clock related functions */
    fmi3GetClockTYPE*             fmi3GetClock;
    fmi3SetClockTYPE*             fmi3SetClock;
    fmi3GetIntervalDecimalTYPE*   fmi3GetIntervalDecimal;
    fmi3GetIntervalFractionTYPE*  fmi3GetIntervalFraction;
    fmi3SetIntervalDecimalTYPE*   fmi3SetIntervalDecimal;
    fmi3SetIntervalFractionTYPE*  fmi3SetIntervalFraction;
    fmi3UpdateDiscreteStatesTYPE* fmi3UpdateDiscreteStates;

    /***************************************************
    Functions for Model Exchange
    ****************************************************/

    fmi3EnterContinuousTimeModeTYPE* fmi3EnterContinuousTimeMode;
    fmi3CompletedIntegratorStepTYPE* fmi3CompletedIntegratorStep;

    /* Providing independent variables and re-initialization of caching */
    fmi3SetTimeTYPE*             fmi3SetTime;
    fmi3SetContinuousStatesTYPE* fmi3SetContinuousStates;

    /* Evaluation of the model equations */
    fmi3GetContinuousStateDerivativesTYPE* fmi3GetContinuousStateDerivatives;
    fmi3GetEventIndicatorsTYPE*            fmi3GetEventIndicators;
    fmi3GetContinuousStatesTYPE*           fmi3GetContinuousStates;
    fmi3GetNominalsOfContinuousStatesTYPE* fmi3GetNominalsOfContinuousStates;
    fmi3GetNumberOfEventIndicatorsTYPE*    fmi3GetNumberOfEventIndicators;
    fmi3GetNumberOfContinuousStatesTYPE*   fmi3GetNumberOfContinuousStates;

    /***************************************************
    Functions for Co-Simulation
    ****************************************************/

    /* Simulating the FMU */
    fmi3EnterStepModeTYPE*          fmi3EnterStepMode;
    fmi3GetOutputDerivativesTYPE*   fmi3GetOutputDerivatives;
    fmi3ActivateModelPartitionTYPE* fmi3ActivateModelPartition;
    fmi3DoStepTYPE*                 fmi3DoStep;

} FMU;


#ifdef _WIN32
#define LOAD_SYMBOL(f) if (!(S->##f = GetProcAddress(S->libraryHandle, #f))) goto FAIL;
#else
#define LOAD_SYMBOL(f) if (!(S->##f = dlsym(S->libraryHandle, #f))) goto FAIL;
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

int main(int argc, char* argv[]) {

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

    fmi3Status status = fmi3OK;
    const fmi3Float64 fixedStep = FIXED_STEP;
    const fmi3Float64 stopTime = STOP_TIME;
    fmi3Float64 time = 0;
    const fmi3Float64 tStart = 0;
    fmi3Int32 rootsFound[NZ] = { 0 };
    fmi3Instance m = NULL;
    fmi3Float64 x[NX] = { 0 };
    fmi3Float64 x_nominal[NX] = { 0 };
    fmi3Float64 der_x[NX] = { 0 };
    fmi3Float64 z[NZ] = { 0 };
    fmi3Float64 previous_z[NZ] = { 0 };
    FILE *outputFile = NULL;

    printf("Running " xstr(MODEL_IDENTIFIER) " as Model Exchange... \n");

    outputFile = fopen(xstr(MODEL_IDENTIFIER) "_me.csv", "w");

    if (!outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    // write the header of the CSV
    fputs(OUTPUT_FILE_HEADER, outputFile);

    // tag::ModelExchange[]
    m = S->fmi3InstantiateModelExchange("m", INSTANTIATION_TOKEN, NULL, fmi3False, fmi3False, NULL, cb_logMessage);
    // "m" is the instance name
    // "M_" is the MODEL_IDENTIFIER

    if (m == NULL) {
        status = fmi3Error;
        goto TERMINATE;
    }

    // set the start time
    time = tStart;

    // set all variable start values (of "ScalarVariable / <type> / start") and
    // set the start values at time = Tstart
    // M_fmi3SetReal/Integer/Boolean/String(m, ...)

    // initialize
    // determine continuous and discrete states
    CHECK_STATUS(S->fmi3EnterInitializationMode(m, fmi3False, 0.0, tStart, fmi3True, stopTime));

    // TODO: apply input

    CHECK_STATUS(S->fmi3ExitInitializationMode(m));

    fmi3Boolean inputEvent = fmi3False;
    fmi3Boolean timeEvent = fmi3False;
    fmi3Boolean stateEvent = fmi3False;
    fmi3Boolean stepEvent = fmi3False;

    fmi3Boolean discreteStatesNeedUpdate = fmi3True;
    fmi3Boolean terminateSimulation = fmi3False;
    fmi3Boolean nominalsOfContinuousStatesChanged = fmi3False;
    fmi3Boolean valuesOfContinuousStatesChanged = fmi3False;
    fmi3Boolean nextEventTimeDefined = fmi3False;
    fmi3Float64 nextEventTime = INFINITY;

    // intial event iteration
    while (discreteStatesNeedUpdate) {

        CHECK_STATUS(S->fmi3UpdateDiscreteStates(
            m,
            &discreteStatesNeedUpdate,
            &terminateSimulation,
            &nominalsOfContinuousStatesChanged,
            &valuesOfContinuousStatesChanged,
            &nextEventTimeDefined,
            &nextEventTime));

        if (terminateSimulation) {
            goto TERMINATE;
        }
    }

    CHECK_STATUS(S->fmi3EnterContinuousTimeMode(m));

    // initialize previous event indicators
    CHECK_STATUS(S->fmi3GetEventIndicators(m, previous_z, NZ));

    // retrieve initial state x and
    // nominal values of x (if absolute tolerance is needed)
    CHECK_STATUS(S->fmi3GetContinuousStates(m, x, NX));
    CHECK_STATUS(S->fmi3GetNominalsOfContinuousStates(m, x_nominal, NX));

    // retrieve solution at t=Tstart, for example, for outputs
    // S->fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

    while (!terminateSimulation) {

        // detect time event
        timeEvent = nextEventTimeDefined && time >= nextEventTime;

        // handle events
        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            CHECK_STATUS(S->fmi3EnterEventMode(m, stepEvent, stateEvent, rootsFound, NZ, timeEvent));

            nominalsOfContinuousStatesChanged = fmi3False;
            valuesOfContinuousStatesChanged = fmi3False;

            // event iteration
            do {
                // set inputs at super dense time point
                // S->fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;

                // update discrete states
                CHECK_STATUS(S->fmi3UpdateDiscreteStates(m, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

                // get output at super dense time point
                // S->fmi3GetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

                nominalsOfContinuousStatesChanged |= nominalsChanged;
                valuesOfContinuousStatesChanged |= statesChanged;

                if (terminateSimulation) {
                    goto TERMINATE;
                }

            } while (discreteStatesNeedUpdate);

            // enter Continuous-Time Mode
            CHECK_STATUS(S->fmi3EnterContinuousTimeMode(m));

            // retrieve solution at simulation (re)start
            CHECK_STATUS(recordVariables(outputFile, S, m, time));

            if (valuesOfContinuousStatesChanged) {
                // the model signals a value change of states, retrieve them
                CHECK_STATUS(S->fmi3GetContinuousStates(m, x, NX));
            }

            if (nominalsOfContinuousStatesChanged) {
                // the meaning of states has changed; retrieve new nominal values
                CHECK_STATUS(S->fmi3GetNominalsOfContinuousStates(m, x_nominal, NX));
            }

        }

        if (time >= stopTime) {
            goto TERMINATE;
        }

        // compute derivatives
        CHECK_STATUS(S->fmi3GetContinuousStateDerivatives(m, der_x, NX));

        // advance time
        time += fixedStep;

        CHECK_STATUS(S->fmi3SetTime(m, time));

        // set continuous inputs at t = time
        // S->fmi3SetFloat*(m, ...)

        // set states at t = time and perform one step
        for (size_t i = 0; i < NX; i++) {
            x[i] += fixedStep * der_x[i]; // forward Euler method
        }

        CHECK_STATUS(S->fmi3SetContinuousStates(m, x, NX));

        // get event indicators at t = time
        CHECK_STATUS(S->fmi3GetEventIndicators(m, z, NZ));

        stateEvent = fmi3False;

        for (size_t i = 0; i < NZ; i++) {

            // check for zero crossings
            if (previous_z[i] < 0 && z[i] >= 0) {
                rootsFound[i] = 1;   // -\+
            } else  if (previous_z[i] > 0 && z[i] <= 0) {
                rootsFound[i] = -1;  // +/-
            } else {
                rootsFound[i] = 0;   // no zero crossing
            }

            stateEvent |= rootsFound[i];

            previous_z[i] = z[i]; // remember the current value
        }

        // inform the model about an accepted step
        CHECK_STATUS(S->fmi3CompletedIntegratorStep(m, fmi3True, &stepEvent, &terminateSimulation));

        // get continuous output
        // S->fmi3GetFloat*(m, ...)
        CHECK_STATUS(recordVariables(outputFile, S, m, time));
    }

TERMINATE:

    if (m && status != fmi3Error && status != fmi3Fatal) {
        // retrieve final values and terminate simulation
        CHECK_STATUS(recordVariables(outputFile, S, m, time));
        fmi3Status s = S->fmi3Terminate(m);
        status = max(status, s);
    }

    if (m && status != fmi3Fatal) {
        // clean up
        S->fmi3FreeInstance(m);
    }
    // end::ModelExchange[]

    printf("done.\n");

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
