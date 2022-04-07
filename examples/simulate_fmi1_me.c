#include <math.h>

#define OUTPUT_FILE  xstr(MODEL_IDENTIFIER) "_me_out.csv"
#define LOG_FILE     xstr(MODEL_IDENTIFIER) "_me_log.txt"
#define SIMULATE_MODEL_EXCHANGE

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    const fmi1Real fixedStep = FIXED_SOLVER_STEP;
    const fmi1Real stopTime  = DEFAULT_STOP_TIME;

#if NZ > 0
    fmi1Integer rootsFound[NZ] = { 0 };
    fmi1Real z[NZ] = { 0 };
    fmi1Real previous_z[NZ] = { 0 };
#else
    fmi1Integer *rootsFound = NULL;
#endif

#if NX > 0
    fmi1Real x[NX] = { 0 };
    fmi1Real x_nominal[NX] = { 0 };
    fmi1Real der_x[NX] = { 0 };
#endif

    fmi1Boolean inputEvent = fmi1False;
    fmi1Boolean timeEvent  = fmi1False;
    fmi1Boolean stateEvent = fmi1False;
    fmi1Boolean stepEvent  = fmi1False;

    CALL(FMI1InstantiateModel(S,
        xstr(MODEL_IDENTIFIER), // modelIdentifier
        INSTANTIATION_TOKEN,    // GUID
        fmi1False               // loggingOn
    ));

    // set the start time
    fmi1Real time = 0;

    // set start values
    CALL(applyStartValues(S));

    // initialize
    CALL(applyContinuousInputs(S, false));
    CALL(applyDiscreteInputs(S));

    CALL(FMI1Initialize(S, fmi1False, 0.0));

    fmi1EventInfo eventInfo = { 0 };

#if NZ > 0
    // initialize previous event indicators
    CALL(FMI1GetEventIndicators(S, previous_z, NZ));
#endif

#if NX > 0
    // retrieve initial state x and
    // nominal values of x (if absolute tolerance is needed)
    CALL(FMI1GetContinuousStates(S, x, NX));
    CALL(FMI1GetNominalContinuousStates(S, x_nominal, NX));
#endif

    // retrieve solution at t=Tstart, for example, for outputs

    CALL(recordVariables(S, outputFile));

    int steps = 0;

    while (!eventInfo.terminateSimulation) {

        // detect input and time events
        inputEvent = time >= nextInputEventTime(time);
        timeEvent = eventInfo.upcomingTimeEvent && time >= eventInfo.nextEventTime;

        const bool eventOccurred = inputEvent || timeEvent || stateEvent || stepEvent;

        // handle events
        if (eventOccurred) {

            if (inputEvent) {
                CALL(applyContinuousInputs(S, true));
                CALL(applyDiscreteInputs(S));
            }

            fmi1Boolean nominalsChanged = fmi1False;
            fmi1Boolean statesChanged   = fmi1False;

            // event iteration
            do {
                CALL(FMI1EventUpdate(S, fmi1True, &eventInfo));

                nominalsChanged |= eventInfo.stateValueReferencesChanged;
                statesChanged   |= eventInfo.stateValuesChanged;

                if (eventInfo.terminateSimulation) {
                    goto TERMINATE;
                }
            } while (!eventInfo.iterationConverged);

            // retrieve solution at simulation (re)start
            CALL(recordVariables(S, outputFile));

#if NX > 0
            if (statesChanged) {
                // the model signals a value change of states, retrieve them
                CALL(FMI1GetContinuousStates(S, x, NX));
            }

            if (nominalsChanged) {
                // the meaning of states has changed; retrieve new nominal values
                CALL(FMI1GetNominalContinuousStates(S, x_nominal, NX));
            }
#endif
        }

        if (time >= stopTime) {
            goto TERMINATE;
        }

#if NX > 0
        // compute continous state derivatives
        CALL(FMI1GetDerivatives(S, der_x, NX));
#endif
        // advance time
        time = ++steps * fixedStep;

        CALL(FMI1SetTime(S, time));

        // apply continuous inputs
        CALL(applyContinuousInputs(S, false));

#if NX > 0
        // set states at t = time and perform one step
        for (size_t i = 0; i < NX; i++) {
            x[i] += fixedStep * der_x[i]; // forward Euler method
        }

        CALL(FMI1SetContinuousStates(S, x, NX));
#endif

#if NZ > 0
        stateEvent = fmi1False;

        if (eventOccurred) {
            // reset the previous event indicators
            CALL(FMI1GetEventIndicators(S, previous_z, NZ));
        } else {
            // get event indicators at t = time
            CALL(FMI1GetEventIndicators(S, z, NZ));

            for (size_t i = 0; i < NZ; i++) {

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
#endif

        // inform the model about an accepted step
        CALL(FMI1CompletedIntegratorStep(S, &stepEvent));

        if (eventInfo.terminateSimulation) {
            goto TERMINATE;
        }

        // get continuous output
        CALL(recordVariables(S, outputFile));
    }

TERMINATE:
    return tearDown();
}
