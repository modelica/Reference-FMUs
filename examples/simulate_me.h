#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#define FMI3_FUNCTION_PREFIX M_
#include "fmi3Functions.h"
#undef FMI3_FUNCTION_PREFIX

#include "util.h"
#include "config.h"

#define xstr(s) str(s)
#define str(s) #s

fmi3Status recordVariables(FILE *outputFile, fmi3Instance s, fmi3Float64 time);

int main(int argc, char* argv[]) {

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
    m = M_fmi3InstantiateModelExchange("m", INSTANTIATION_TOKEN, NULL, fmi3False, fmi3False, NULL, cb_logMessage);
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
    CHECK_STATUS(M_fmi3EnterInitializationMode(m, fmi3False, 0.0, tStart, fmi3True, stopTime));

    // TODO: apply input

    CHECK_STATUS(M_fmi3ExitInitializationMode(m));

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

        CHECK_STATUS(M_fmi3UpdateDiscreteStates(
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

    CHECK_STATUS(M_fmi3EnterContinuousTimeMode(m));

    // initialize previous event indicators
    CHECK_STATUS(M_fmi3GetEventIndicators(m, previous_z, NZ));

    // retrieve initial state x and
    // nominal values of x (if absolute tolerance is needed)
    CHECK_STATUS(M_fmi3GetContinuousStates(m, x, NX));
    CHECK_STATUS(M_fmi3GetNominalsOfContinuousStates(m, x_nominal, NX));

    // retrieve solution at t=Tstart, for example, for outputs
    // M_fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

    while (!terminateSimulation) {

        // detect time event
        timeEvent = nextEventTimeDefined && time >= nextEventTime;

        // handle events
        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            CHECK_STATUS(M_fmi3EnterEventMode(m, stepEvent, stateEvent, rootsFound, NZ, timeEvent));

            nominalsOfContinuousStatesChanged = fmi3False;
            valuesOfContinuousStatesChanged = fmi3False;

            // event iteration
            do {
                // set inputs at super dense time point
                // M_fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;

                // update discrete states
                CHECK_STATUS(M_fmi3UpdateDiscreteStates(m, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

                // get output at super dense time point
                // M_fmi3GetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

                nominalsOfContinuousStatesChanged |= nominalsChanged;
                valuesOfContinuousStatesChanged |= statesChanged;

                if (terminateSimulation) {
                    goto TERMINATE;
                }

            } while (discreteStatesNeedUpdate);

            // enter Continuous-Time Mode
            CHECK_STATUS(M_fmi3EnterContinuousTimeMode(m));

            // retrieve solution at simulation (re)start
            CHECK_STATUS(recordVariables(outputFile, m, time));

            if (valuesOfContinuousStatesChanged) {
                // the model signals a value change of states, retrieve them
                CHECK_STATUS(M_fmi3GetContinuousStates(m, x, NX));
            }

            if (nominalsOfContinuousStatesChanged) {
                // the meaning of states has changed; retrieve new nominal values
                CHECK_STATUS(M_fmi3GetNominalsOfContinuousStates(m, x_nominal, NX));
            }

        }

        if (time >= stopTime) {
            goto TERMINATE;
        }

        // compute derivatives
        CHECK_STATUS(M_fmi3GetContinuousStateDerivatives(m, der_x, NX));

        // advance time
        time += fixedStep;

        CHECK_STATUS(M_fmi3SetTime(m, time));

        // set continuous inputs at t = time
        // M_fmi3SetFloat*(m, ...)

        // set states at t = time and perform one step
        for (size_t i = 0; i < NX; i++) {
            x[i] += fixedStep * der_x[i]; // forward Euler method
        }

        CHECK_STATUS(M_fmi3SetContinuousStates(m, x, NX));

        // get event indicators at t = time
        CHECK_STATUS(M_fmi3GetEventIndicators(m, z, NZ));

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
        CHECK_STATUS(M_fmi3CompletedIntegratorStep(m, fmi3True, &stepEvent, &terminateSimulation));

        // get continuous output
        // M_fmi3GetFloat*(m, ...)
        CHECK_STATUS(recordVariables(outputFile, m, time));
    }

TERMINATE:

    if (m && status != fmi3Error && status != fmi3Fatal) {
        // retrieve final values and terminate simulation
        CHECK_STATUS(recordVariables(outputFile, m, time));
        fmi3Status s = M_fmi3Terminate(m);
        status = max(status, s);
    }

    if (m && status != fmi3Fatal) {
        // clean up
        M_fmi3FreeInstance(m);
    }
    // end::ModelExchange[]

    printf("done.\n");

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
