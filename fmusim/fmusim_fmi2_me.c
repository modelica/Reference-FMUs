#include <stdlib.h>
#include <math.h>
#include "fmusim_fmi2.h"
#include "fmusim_fmi2_me.h"



#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI2ME(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourcePath,
    FMISimulationResult* result,
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[],
    double startTime,
    double stepSize,
    double stopTime) {

    const size_t nx = modelDescription->nContinuousStates;
    const size_t nz = modelDescription->nEventIndicators;
    
    const fmi2Real fixedStep = 1e-2;

    fmi2Integer*   rootsFound = (fmi2Integer*)  calloc(nz, sizeof(fmi2Integer));
    fmi2Real* z          = (fmi2Real*)calloc(nz, sizeof(fmi2Real));
    fmi2Real* previous_z = (fmi2Real*)calloc(nz, sizeof(fmi2Real));

    fmi2Real* x         = (fmi2Real*)calloc(nx, sizeof(fmi2Real));
    fmi2Real* x_nominal = (fmi2Real*)calloc(nx, sizeof(fmi2Real));
    fmi2Real* der_x     = (fmi2Real*)calloc(nx, sizeof(fmi2Real));

    FMIStatus status = FMIOK;

    fmi2Boolean inputEvent = fmi2False;
    fmi2Boolean timeEvent  = fmi2False;
    fmi2Boolean stateEvent = fmi2False;
    fmi2Boolean stepEvent  = fmi2False;

    fmi2Boolean discreteStatesNeedUpdate          = fmi2True;
    fmi2Boolean terminateSimulation               = fmi2False;
    fmi2Boolean nominalsOfContinuousStatesChanged = fmi2False;
    fmi2Boolean valuesOfContinuousStatesChanged   = fmi2False;
    fmi2Boolean nextEventTimeDefined              = fmi2False;
    fmi2Real nextEventTime                     = INFINITY;
    fmi2Real nextInputEvent                    = INFINITY;

    //CALL(FMI2InstantiateModelExchange(S,
    //    modelDescription->instantiationToken,  // instantiationToken
    //    resourcePath,                          // resourcePath
    //    fmi2False,                             // visible
    //    fmi2False                              // loggingOn
    //));

    //// set the start time
    //fmi2Real time = 0;

    //// set start values
    //CALL(applyStartValuesFMI3(S, nStartValues, startVariables, startValues));

    //// initialize
    //// determine continuous and discrete states
    //CALL(FMI2EnterInitializationMode(S, fmi2False, 0.0, time, fmi2True, stopTime));

    ////CALL(applyContinuousInputs(S, false));
    ////CALL(applyDiscreteInputs(S));

    //CALL(FMI2ExitInitializationMode(S));

    //// intial event iteration
    //while (discreteStatesNeedUpdate) {

    //    CALL(FMI2UpdateDiscreteStates(S,
    //        &discreteStatesNeedUpdate,
    //        &terminateSimulation,
    //        &nominalsOfContinuousStatesChanged,
    //        &valuesOfContinuousStatesChanged,
    //        &nextEventTimeDefined,
    //        &nextEventTime));

    //    if (terminateSimulation) {
    //        goto TERMINATE;
    //    }
    //}

    //CALL(FMI2EnterContinuousTimeMode(S));

    //if (nz > 0) {
    //    // initialize previous event indicators
    //    CALL(FMI2GetEventIndicators(S, previous_z, nz));
    //}

    //if (nx > 0) {
    //    // retrieve initial state x and
    //    // nominal values of x (if absolute tolerance is needed)
    //    CALL(FMI2GetContinuousStates(S, x, nx));
    //    CALL(FMI2GetNominalsOfContinuousStates(S, x_nominal, nx));
    //}

    //// retrieve solution at t=Tstart, for example, for outputs
    //// S->fmi2SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

    //CALL(FMISample(S, time, result));

    //uint64_t step = 0;

    //while (!terminateSimulation) {

    //    // detect input and time events
    //    inputEvent = fmi2False; // time >= nextInputEventTime(time);
    //    timeEvent = nextEventTimeDefined && time >= nextEventTime;

    //    const bool eventOccurred = inputEvent || timeEvent || stateEvent || stepEvent;

    //    // handle events
    //    if (eventOccurred) {

    //        CALL(FMI2EnterEventMode(S));

    //        if (inputEvent) {
    //            //CALL(applyContinuousInputs(S, true));
    //            //CALL(applyDiscreteInputs(S));
    //        }

    //        nominalsOfContinuousStatesChanged = fmi2False;
    //        valuesOfContinuousStatesChanged = fmi2False;

    //        // event iteration
    //        do {
    //            // set inputs at super dense time point
    //            // S->fmi2SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

    //            fmi2Boolean nominalsChanged = fmi2False;
    //            fmi2Boolean statesChanged = fmi2False;

    //            // update discrete states
    //            CALL(FMI2UpdateDiscreteStates(S, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

    //            // get output at super dense time point
    //            // S->fmi2GetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

    //            nominalsOfContinuousStatesChanged |= nominalsChanged;
    //            valuesOfContinuousStatesChanged |= statesChanged;

    //            if (terminateSimulation) {
    //                goto TERMINATE;
    //            }

    //        } while (discreteStatesNeedUpdate);

    //        // enter Continuous-Time Mode
    //        CALL(FMI2EnterContinuousTimeMode(S));

    //        // retrieve solution at simulation (re)start
    //        CALL(FMISample(S, time, result));

    //        if (nx > 0) {
    //            
    //            if (valuesOfContinuousStatesChanged) {
    //                // the model signals a value change of states, retrieve them
    //                CALL(FMI2GetContinuousStates(S, x, nx));
    //            }

    //            if (nominalsOfContinuousStatesChanged) {
    //                // the meaning of states has changed; retrieve new nominal values
    //                CALL(FMI2GetNominalsOfContinuousStates(S, x_nominal, nx));
    //            }
    //        }
    //    }

    //    if (time >= stopTime) {
    //        goto TERMINATE;
    //    }

    //    if (nx > 0) {
    //        // compute continuous state derivatives
    //        CALL(FMI2GetContinuousStateDerivatives(S, der_x, nx));
    //    }

    //    // advance time
    //    time = ++step * fixedStep;

    //    CALL(FMI2SetTime(S, time));

    //    // apply continuous inputs
    //    //CALL(applyContinuousInputs(S, false));

    //    if (nx > 0) {
    //        // set states at t = time and perform one step
    //        for (size_t i = 0; i < nx; i++) {
    //            x[i] += fixedStep * der_x[i]; // forward Euler method
    //        }

    //        CALL(FMI2SetContinuousStates(S, x, nx));
    //    }

    //    if (nz > 0) {

    //        stateEvent = fmi2False;

    //        if (eventOccurred) {
    //            // reset the previous event indicators
    //            CALL(FMI2GetEventIndicators(S, previous_z, nz));
    //        } else {
    //            // get event indicators at t = time
    //            CALL(FMI2GetEventIndicators(S, z, nz));

    //            for (size_t i = 0; i < nz; i++) {

    //                // check for zero crossings
    //                if (previous_z[i] <= 0 && z[i] > 0) {
    //                    rootsFound[i] = 1;   // -\+
    //                } else  if (previous_z[i] > 0 && z[i] <= 0) {
    //                    rootsFound[i] = -1;  // +/-
    //                } else {
    //                    rootsFound[i] = 0;   // no zero crossing
    //                }

    //                stateEvent |= rootsFound[i];

    //                previous_z[i] = z[i]; // remember the current value
    //            }
    //        }
    //    }

    //    // inform the model about an accepted step
    //    CALL(FMI2CompletedIntegratorStep(S, fmi2True, &stepEvent, &terminateSimulation));

    //    // get continuous output
    //    CALL(FMISample(S, time, result));
    //}

TERMINATE:
    return status;
}