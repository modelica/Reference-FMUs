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

#define FIXED_STEP 1e-2
#define STOP_TIME 3
#define OUTPUT_FILE_HEADER "time,h,v\n"

fmi3Status recordVariables(FILE *outputFile, fmi3Instance s, fmi3Float64 time) {
    const fmi3ValueReference valueReferences[2] = { vr_h, vr_v };
    fmi3Float64 values[2] = { 0 };
    fmi3Status status = M_fmi3GetFloat64(s, valueReferences, 2, values, 2);
    fprintf(outputFile, "%g,%g,%g\n", time, values[0], values[1]);
    return status;
}

int main(int argc, char* argv[]) {

fmi3Status status = fmi3OK;
const fmi3Float64 fixedStep = FIXED_STEP;
fmi3Float64 h = fixedStep;
fmi3Float64 tNext = h;
const fmi3Float64 tEnd = STOP_TIME;
fmi3Float64 time = 0;
const fmi3Float64 tStart = 0;
fmi3Boolean timeEvent, stateEvent, enterEventMode, terminateSimulation = fmi3False, initialEventMode;
fmi3Int32 rootsFound[NZ] = { 0 };
fmi3Instance m = NULL;
fmi3Float64 x[NX] = { 0 };
fmi3Float64 x_nominal[NX] = { 0 };
fmi3Float64 der_x[NX] = { 0 };
fmi3Float64 z[NZ] = { 0 };
fmi3Float64 previous_z[NZ] = { 0 };
FILE *outputFile = NULL;

printf("Running model_exchange example... ");

outputFile = fopen("model_exchange_out.csv", "w");

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
time  = tStart;

// set all variable start values (of "ScalarVariable / <type> / start") and
// set the start values at time = Tstart
// M_fmi3SetReal/Integer/Boolean/String(m, ...)

// initialize
// determine continuous and discrete states
CHECK_STATUS(M_fmi3EnterInitializationMode(m, fmi3False, 0.0, tStart, fmi3True, tEnd));
CHECK_STATUS(M_fmi3ExitInitializationMode(m));

initialEventMode = fmi3True;
enterEventMode   = fmi3False;
timeEvent        = fmi3False;
stateEvent       = fmi3False;

// initialize previous event indicators
CHECK_STATUS(M_fmi3GetEventIndicators(m, previous_z, NZ));

initialEventMode = fmi3False;

CHECK_STATUS(M_fmi3EnterContinuousTimeMode(m));

// retrieve initial state x and
// nominal values of x (if absolute tolerance is needed)
CHECK_STATUS(M_fmi3GetContinuousStates(m, x, NX));
CHECK_STATUS(M_fmi3GetNominalsOfContinuousStates(m, x_nominal, NX));

// retrieve solution at t=Tstart, for example, for outputs
// M_fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

while (!terminateSimulation) {

    tNext = time + h;

    // handle events
    if (enterEventMode || stateEvent || timeEvent) {

        if (!initialEventMode) {
            CHECK_STATUS(M_fmi3EnterEventMode(m, fmi3False, rootsFound, NZ, timeEvent));
        }

        // event iteration
        fmi3Boolean newDiscreteStatesNeeded           = fmi3True;
        fmi3Boolean terminateSimulation               = fmi3False;
        fmi3Boolean nominalsOfContinuousStatesChanged = fmi3False;
        fmi3Boolean valuesOfContinuousStatesChanged   = fmi3False;
        fmi3Boolean nextEventTimeDefined              = fmi3False;
        fmi3Float64 nextEventTime                     = 0;

        while (newDiscreteStatesNeeded) {

            // set inputs at super dense time point
            // M_fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

            fmi3Boolean nominalsChanged = fmi3False;
            fmi3Boolean statesChanged   = fmi3False;

            // update discrete states
            CHECK_STATUS(M_fmi3NewDiscreteStates(m, &newDiscreteStatesNeeded, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

            // getOutput at super dense time point
            // M_fmi3GetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

            nominalsOfContinuousStatesChanged |= nominalsChanged;
            valuesOfContinuousStatesChanged   |= statesChanged;

            if (terminateSimulation) goto TERMINATE;
        }

        // enter Continuous-Time Mode
        CHECK_STATUS(M_fmi3EnterContinuousTimeMode(m));

        // retrieve solution at simulation (re)start
        CHECK_STATUS(recordVariables(outputFile, m, time));

        if (initialEventMode || valuesOfContinuousStatesChanged) {
            // the model signals a value change of states, retrieve them
            CHECK_STATUS(M_fmi3GetContinuousStates(m, x, NX));
        }

        if (initialEventMode || nominalsOfContinuousStatesChanged) {
            // the meaning of states has changed; retrieve new nominal values
            CHECK_STATUS(M_fmi3GetNominalsOfContinuousStates(m, x_nominal, NX));
        }

        if (nextEventTimeDefined) {
            tNext = min(nextEventTime, tEnd);
        } else {
            tNext = tEnd;
        }

        initialEventMode = fmi3False;
    }

    if (time >= tEnd) {
        goto TERMINATE;
    }

    // compute derivatives
    CHECK_STATUS(M_fmi3GetDerivatives(m, der_x, NX));

    // advance time
    h = min(fixedStep, tNext - time);
    time += h;
    CHECK_STATUS(M_fmi3SetTime(m, time));

    // set continuous inputs at t = time
    // M_fmi3SetFloat*(m, ...)

    // set states at t = time and perform one step
    for (size_t i = 0; i < NX; i++) {
        x[i] += h * der_x[i]; // forward Euler method
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
    CHECK_STATUS(M_fmi3CompletedIntegratorStep(m, fmi3True, &enterEventMode, &terminateSimulation));

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
