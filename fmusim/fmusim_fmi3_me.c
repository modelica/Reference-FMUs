#include <stdlib.h>
#include <math.h>
#include "fmusim_fmi3.h"
#include "fmusim_fmi3_me.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI3ME(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourcePath,
    FMISimulationResult* result,
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[],
    double startTime,
    double stepSize,
    double stopTime,
    const FMUStaticInput* input) {

    const size_t nx = modelDescription->nContinuousStates;
    const size_t nz = modelDescription->nEventIndicators;
    
    fmi3Int32*   rootsFound = (fmi3Int32*)  calloc(nz, sizeof(fmi3Int32));
    fmi3Float64* z          = (fmi3Float64*)calloc(nz, sizeof(fmi3Float64));
    fmi3Float64* previous_z = (fmi3Float64*)calloc(nz, sizeof(fmi3Float64));

    fmi3Float64* x         = (fmi3Float64*)calloc(nx, sizeof(fmi3Float64));
    fmi3Float64* x_nominal = (fmi3Float64*)calloc(nx, sizeof(fmi3Float64));
    fmi3Float64* der_x     = (fmi3Float64*)calloc(nx, sizeof(fmi3Float64));

    FMIStatus status = FMIOK;

    fmi3Boolean inputEvent = fmi3False;
    fmi3Boolean timeEvent  = fmi3False;
    fmi3Boolean stateEvent = fmi3False;
    fmi3Boolean stepEvent  = fmi3False;

    fmi3Boolean discreteStatesNeedUpdate          = fmi3True;
    fmi3Boolean terminateSimulation               = fmi3False;
    fmi3Boolean nominalsOfContinuousStatesChanged = fmi3False;
    fmi3Boolean valuesOfContinuousStatesChanged   = fmi3False;
    fmi3Boolean nextEventTimeDefined              = fmi3False;
    fmi3Float64 nextEventTime                     = INFINITY;
    fmi3Float64 nextInputEvent                    = INFINITY;

    CALL(FMI3InstantiateModelExchange(S,
        modelDescription->instantiationToken,  // instantiationToken
        resourcePath,                          // resourcePath
        fmi3False,                             // visible
        fmi3False                              // loggingOn
    ));

    fmi3Float64 time = startTime;

    // set start values
    CALL(applyStartValuesFMI3(S, nStartValues, startVariables, startValues));
    CALL(FMIApplyInput(S, input, time, 
        true,  // discrete
        true,  // continous
        false  // after event
    ));

    // initialize
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, stopTime));
    CALL(FMI3ExitInitializationMode(S));

    // intial event iteration
    while (discreteStatesNeedUpdate) {

        CALL(FMI3UpdateDiscreteStates(S,
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

    CALL(FMI3EnterContinuousTimeMode(S));

    if (nz > 0) {
        // initialize previous event indicators
        CALL(FMI3GetEventIndicators(S, previous_z, nz));
    }

    if (nx > 0) {
        // retrieve initial state x and
        // nominal values of x (if absolute tolerance is needed)
        CALL(FMI3GetContinuousStates(S, x, nx));
        CALL(FMI3GetNominalsOfContinuousStates(S, x_nominal, nx));
    }

    // retrieve solution at t=Tstart, for example, for outputs
    // S->fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

    CALL(FMISample(S, time, result));

    uint64_t step = 0;

    while (!terminateSimulation) {

        // detect input and time events
        inputEvent = time >= FMINextInputEvent(input, time);
        timeEvent = nextEventTimeDefined && time >= nextEventTime;

        const bool eventOccurred = inputEvent || timeEvent || stateEvent || stepEvent;

        // handle events
        if (eventOccurred) {

            CALL(FMI3EnterEventMode(S));

            if (inputEvent) {
                CALL(FMIApplyInput(S, input, time, 
                    true,  // discrete
                    true,  // continous
                    true   // after event
                ));
            }

            nominalsOfContinuousStatesChanged = fmi3False;
            valuesOfContinuousStatesChanged = fmi3False;

            // event iteration
            do {
                // set inputs at super dense time point
                // S->fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;

                // update discrete states
                CALL(FMI3UpdateDiscreteStates(S, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

                // get output at super dense time point
                // S->fmi3GetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

                nominalsOfContinuousStatesChanged |= nominalsChanged;
                valuesOfContinuousStatesChanged |= statesChanged;

                if (terminateSimulation) {
                    goto TERMINATE;
                }

            } while (discreteStatesNeedUpdate);

            // enter Continuous-Time Mode
            CALL(FMI3EnterContinuousTimeMode(S));

            // retrieve solution at simulation (re)start
            CALL(FMISample(S, time, result));

            if (nx > 0) {
                
                if (valuesOfContinuousStatesChanged) {
                    // the model signals a value change of states, retrieve them
                    CALL(FMI3GetContinuousStates(S, x, nx));
                }

                if (nominalsOfContinuousStatesChanged) {
                    // the meaning of states has changed; retrieve new nominal values
                    CALL(FMI3GetNominalsOfContinuousStates(S, x_nominal, nx));
                }
            }
        }

        if (time >= stopTime) {
            goto TERMINATE;
        }

        if (nx > 0) {
            // compute continuous state derivatives
            CALL(FMI3GetContinuousStateDerivatives(S, der_x, nx));
        }

        // advance time
        time = startTime + (++step) * stepSize;

        CALL(FMI3SetTime(S, time));

        // apply continuous inputs
        CALL(FMIApplyInput(S, input, time,
            false,  // discrete
            true,   // continous
            false   // after event
        ));

        if (nx > 0) {
            // set states at t = time and perform one step
            for (size_t i = 0; i < nx; i++) {
                x[i] += stepSize * der_x[i]; // forward Euler method
            }

            CALL(FMI3SetContinuousStates(S, x, nx));
        }

        if (nz > 0) {

            stateEvent = fmi3False;

            if (eventOccurred) {
                // reset the previous event indicators
                CALL(FMI3GetEventIndicators(S, previous_z, nz));
            } else {
                // get event indicators at t = time
                CALL(FMI3GetEventIndicators(S, z, nz));

                for (size_t i = 0; i < nz; i++) {

                    // check for zero crossings
                    if (previous_z[i] <= 0 && z[i] > 0) {
                        rootsFound[i] = 1;   // -\+
                    } else  if (previous_z[i] > 0 && z[i] <= 0) {
                        rootsFound[i] = -1;  // +/-
                    } else {
                        rootsFound[i] = 0;   // no zero crossing
                    }

                    stateEvent |= rootsFound[i];

                    previous_z[i] = z[i]; // remember the current value
                }
            }
        }

        // inform the model about an accepted step
        CALL(FMI3CompletedIntegratorStep(S, fmi3True, &stepEvent, &terminateSimulation));

        // get continuous output
        CALL(FMISample(S, time, result));
    }

TERMINATE:
    return status;
}