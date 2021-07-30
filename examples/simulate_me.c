#include <math.h>

#define OUTPUT_FILE  xstr(MODEL_IDENTIFIER) "_me_out.csv"
#define LOG_FILE     xstr(MODEL_IDENTIFIER) "_me_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    const fmi3Float64 fixedStep = FIXED_SOLVER_STEP;
    const fmi3Float64 stopTime = DEFAULT_STOP_TIME;

#if NZ > 0
    fmi3Int32 rootsFound[NZ] = { 0 };
    fmi3Float64 z[NZ] = { 0 };
    fmi3Float64 previous_z[NZ] = { 0 };
#else
    fmi3Int32 *rootsFound = NULL;
#endif

#if NX > 0
    fmi3Float64 x[NX] = { 0 };
    fmi3Float64 x_nominal[NX] = { 0 };
    fmi3Float64 der_x[NX] = { 0 };
#endif

    printf("Running " xstr(MODEL_IDENTIFIER) " as Model Exchange... \n");

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

    // tag::ModelExchange[]
    CALL(FMI3InstantiateModelExchange(S,
        INSTANTIATION_TOKEN, // instantiationToken
        NULL,                // resourceLocation
        fmi3False,           // visible
        fmi3False            // loggingOn
    ));

    // set the start time
    fmi3Float64 time = 0;

    // TODO: set all variable start values (of "ScalarVariable / <type> / start")

    // initialize
    // determine continuous and discrete states
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, stopTime));

    // TODO: apply input

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

#if NZ > 0
    // initialize previous event indicators
    CALL(FMI3GetEventIndicators(S, previous_z, NZ));
#endif

#if NX > 0
    // retrieve initial state x and
    // nominal values of x (if absolute tolerance is needed)
    CALL(FMI3GetContinuousStates(S, x, NX));
    CALL(FMI3GetNominalsOfContinuousStates(S, x_nominal, NX));
#endif

    // retrieve solution at t=Tstart, for example, for outputs
    // S->fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

    while (!terminateSimulation) {

        // detect time event
        timeEvent = nextEventTimeDefined && time >= nextEventTime;

        // handle events
        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            CALL(FMI3EnterEventMode(S, stepEvent, stateEvent, rootsFound, NZ, timeEvent));

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
            CALL(recordVariables(S, outputFile));

#if NX > 0
            if (valuesOfContinuousStatesChanged) {
                // the model signals a value change of states, retrieve them
                CALL(FMI3GetContinuousStates(S, x, NX));
            }

            if (nominalsOfContinuousStatesChanged) {
                // the meaning of states has changed; retrieve new nominal values
                CALL(FMI3GetNominalsOfContinuousStates(S, x_nominal, NX));
            }
#endif
        }

        if (time >= stopTime) {
            goto TERMINATE;
        }

#if NX > 0
        // compute continous state derivatives
        CALL(FMI3GetContinuousStateDerivatives(S, der_x, NX));
#endif
        // advance time
        time += fixedStep;

        CALL(FMI3SetTime(S, time));

        // set continuous inputs at t = time
        // S->fmi3SetFloat*(m, ...)

#if NX > 0
        // set states at t = time and perform one step
        for (size_t i = 0; i < NX; i++) {
            x[i] += fixedStep * der_x[i]; // forward Euler method
        }

        CALL(FMI3SetContinuousStates(S, x, NX));
#endif

#if NZ > 0
        // get event indicators at t = time
        CALL(FMI3GetEventIndicators(S, z, NZ));

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
#endif

        // inform the model about an accepted step
        CALL(FMI3CompletedIntegratorStep(S, fmi3True, &stepEvent, &terminateSimulation));

        // get continuous output
        // S->fmi3GetFloat*(m, ...)
        CALL(recordVariables(S, outputFile));
    }

TERMINATE:
    return tearDown();
}
