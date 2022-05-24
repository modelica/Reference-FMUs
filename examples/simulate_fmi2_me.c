#include <math.h>

#define OUTPUT_FILE  xstr(MODEL_IDENTIFIER) "_me_out.csv"
#define LOG_FILE     xstr(MODEL_IDENTIFIER) "_me_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    const fmi2Real fixedStep = FIXED_SOLVER_STEP;
    const fmi2Real stopTime  = DEFAULT_STOP_TIME;

#if NZ > 0
    fmi2Integer rootsFound[NZ] = { 0 };
    fmi2Real z[NZ] = { 0 };
    fmi2Real previous_z[NZ] = { 0 };
#else
    fmi2Integer *rootsFound = NULL;
#endif

#if NX > 0
    fmi2Real x[NX] = { 0 };
    fmi2Real x_nominal[NX] = { 0 };
    fmi2Real der_x[NX] = { 0 };
#endif

    fmi2Boolean inputEvent = fmi2False;
    fmi2Boolean timeEvent  = fmi2False;
    fmi2Boolean stateEvent = fmi2False;
    fmi2Boolean stepEvent  = fmi2False;

    CALL(FMI2Instantiate(S,
        resourceURI(),       // fmuResourceLocation
        fmi2ModelExchange,   // fmuType
        INSTANTIATION_TOKEN, // fmuGUID
        fmi2False,           // visible
        fmi2False            // loggingOn
    ));

    // set the start time
    fmi2Real time = 0;

    // set start values
    CALL(applyStartValues(S));

    CALL(FMI2SetupExperiment(S, fmi2False, 0.0, time, fmi2True, stopTime));

    // initialize
    // determine continuous and discrete states
    CALL(FMI2EnterInitializationMode(S));

    CALL(applyContinuousInputs(S, false));
    CALL(applyDiscreteInputs(S));

    CALL(FMI2ExitInitializationMode(S));

    fmi2EventInfo eventInfo = { 0 };

    // intial event iteration
    do {

        CALL(FMI2NewDiscreteStates(S, &eventInfo));

        if (eventInfo.terminateSimulation) {
            goto TERMINATE;
        }
    } while (eventInfo.newDiscreteStatesNeeded);

    CALL(FMI2EnterContinuousTimeMode(S));

#if NZ > 0
    // initialize previous event indicators
    CALL(FMI2GetEventIndicators(S, previous_z, NZ));
#endif

#if NX > 0
    // retrieve initial state x and
    // nominal values of x (if absolute tolerance is needed)
    CALL(FMI2GetContinuousStates(S, x, NX));
    CALL(FMI2GetNominalsOfContinuousStates(S, x_nominal, NX));
#endif

    // retrieve solution at t=Tstart, for example, for outputs

    CALL(recordVariables(S, outputFile));

    int steps = 0;

    while (!eventInfo.terminateSimulation) {

        // detect input and time events
        inputEvent = time >= nextInputEventTime(time);
        timeEvent = eventInfo.nextEventTimeDefined && time >= eventInfo.nextEventTime;

        const bool eventOccurred = inputEvent || timeEvent || stateEvent || stepEvent;

        // handle events
        if (eventOccurred) {

            CALL(FMI2EnterEventMode(S));

            if (inputEvent) {
                CALL(applyContinuousInputs(S, true));
                CALL(applyDiscreteInputs(S));
            }

            fmi2Boolean nominalsChanged = fmi2False;
            fmi2Boolean statesChanged   = fmi2False;

            // event iteration
            do {
                // set inputs at super dense time point

                // update discrete states
                CALL(FMI2NewDiscreteStates(S, &eventInfo));

                // get output at super dense time point

                nominalsChanged |= eventInfo.nominalsOfContinuousStatesChanged;
                statesChanged   |= eventInfo.valuesOfContinuousStatesChanged;

                if (eventInfo.terminateSimulation) {
                    goto TERMINATE;
                }

            } while (eventInfo.newDiscreteStatesNeeded);

            // enter Continuous-Time Mode
            CALL(FMI2EnterContinuousTimeMode(S));

            // retrieve solution at simulation (re)start
            CALL(recordVariables(S, outputFile));

#if NX > 0
            if (statesChanged) {
                // the model signals a value change of states, retrieve them
                CALL(FMI2GetContinuousStates(S, x, NX));
            }

            if (nominalsChanged) {
                // the meaning of states has changed; retrieve new nominal values
                CALL(FMI2GetNominalsOfContinuousStates(S, x_nominal, NX));
            }
#endif
        }

        if (time >= stopTime) {
            goto TERMINATE;
        }

#if NX > 0
        // compute continous state derivatives
        CALL(FMI2GetDerivatives(S, der_x, NX));
#endif
        // advance time
        time = ++steps * fixedStep;

        CALL(FMI2SetTime(S, time));

        // apply continuous inputs
        CALL(applyContinuousInputs(S, false));

#if NX > 0
        // set states at t = time and perform one step
        for (size_t i = 0; i < NX; i++) {
            x[i] += fixedStep * der_x[i]; // forward Euler method
        }

        CALL(FMI2SetContinuousStates(S, x, NX));
#endif

#if NZ > 0
        stateEvent = fmi2False;

        if (eventOccurred) {
            // reset the previous event indicators
            CALL(FMI2GetEventIndicators(S, previous_z, NZ));
        } else {
            // get event indicators at t = time
            CALL(FMI2GetEventIndicators(S, z, NZ));

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
        CALL(FMI2CompletedIntegratorStep(S, fmi2True, &stepEvent, &eventInfo.terminateSimulation));

        if (eventInfo.terminateSimulation) {
            goto TERMINATE;
        }

        // get continuous output
        CALL(recordVariables(S, outputFile));
    }

TERMINATE:
    return tearDown();
}
