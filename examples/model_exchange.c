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

#define CHECK_STATUS(S) status = S; if (status != fmi3OK) goto TERMINATE_MODEL;

int main(int argc, char* argv[]) {

    fmi3Status status = fmi3OK;
    const fmi3Float64 fixedStep = FIXED_STEP;
    fmi3Float64 h = fixedStep;
    fmi3Float64 tNext = h;
    const fmi3Float64 tEnd = STOP_TIME;
    fmi3Float64 time = 0;
    const fmi3Float64 tStart = 0;
    fmi3Boolean timeEvent, stateEvent, enterEventMode, terminateSimulation = fmi3False, initialEventMode, valuesOfContinuousStatesChanged, nominalsOfContinuousStatesChanged;
    fmi3EventInfo eventInfo;
    fmi3Int32 rootsFound[NUMBER_OF_EVENT_INDICATORS] = { 0 };

    fmi3CallbackFunctions callbacks = {
        .allocateMemory     = cb_allocateMemory,
        .freeMemory         = cb_freeMemory,
        .logMessage         = cb_logMessage,
        .intermediateUpdate = NULL,
        .lockPreemption     = NULL,
        .unlockPreemption   = NULL
    };
    
    fmi3Instance m = NULL;
    fmi3Float64 x[NUMBER_OF_STATES] = { 0 };
    fmi3Float64 x_nominal[NUMBER_OF_STATES] = { 0 };
    fmi3Float64 der_x[NUMBER_OF_STATES] = { 0 };
    fmi3Float64 z[NUMBER_OF_EVENT_INDICATORS] = { 0 };
    fmi3Float64 previous_z[NUMBER_OF_EVENT_INDICATORS] = { 0 };
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
m = M_fmi3Instantiate("m", fmi3ModelExchange, MODEL_GUID, NULL, &callbacks, fmi3False, fmi3False, NULL);
// "m" is the instance name
// "M_" is the MODEL_IDENTIFIER
    
    if (m == NULL) {
        status = fmi3Error;
        goto TERMINATE_MODEL;
    }

// set the start time
time  = tStart;

// set all variable start values (of "ScalarVariable / <type> / start") and
// set the start values at time = Tstart
// M_fmi3SetReal/Integer/Boolean/String(m, ...)

// initialize
// determine continuous and discrete states
CHECK_STATUS(M_fmi3SetupExperiment(m, fmi3False, 0.0, tStart, fmi3True, tEnd));
CHECK_STATUS(M_fmi3EnterInitializationMode(m));
CHECK_STATUS(M_fmi3ExitInitializationMode(m));

initialEventMode = fmi3True;
enterEventMode   = fmi3False;
timeEvent        = fmi3False;
stateEvent       = fmi3False;

// initialize previous event indicators
CHECK_STATUS(M_fmi3GetEventIndicators(m, previous_z, NUMBER_OF_EVENT_INDICATORS));

CHECK_STATUS(M_fmi3EnterContinuousTimeMode(m));

// retrieve initial state x and
// nominal values of x (if absolute tolerance is needed)
CHECK_STATUS(M_fmi3GetContinuousStates(m, x, NUMBER_OF_STATES));
CHECK_STATUS(M_fmi3GetNominalsOfContinuousStates(m, x_nominal, NUMBER_OF_STATES));

// retrieve solution at t=Tstart, for example, for outputs
// M_fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

while (!terminateSimulation) {

    tNext = time + h;

    // handle events
    if (enterEventMode || stateEvent || timeEvent) {
        
        if (!initialEventMode) {
            CHECK_STATUS(M_fmi3EnterEventMode(m, fmi3False, fmi3False, NUMBER_OF_EVENT_INDICATORS, rootsFound, timeEvent));
        }

        // event iteration
        eventInfo.newDiscreteStatesNeeded = fmi3True;
        valuesOfContinuousStatesChanged   = fmi3False;
        nominalsOfContinuousStatesChanged = fmi3False;

        while (eventInfo.newDiscreteStatesNeeded) {

            // set inputs at super dense time point
            // M_fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

            // update discrete states
            CHECK_STATUS(M_fmi3NewDiscreteStates(m, &eventInfo));

            // getOutput at super dense time point
            // M_fmi3GetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)
            valuesOfContinuousStatesChanged |= eventInfo.valuesOfContinuousStatesChanged;
            nominalsOfContinuousStatesChanged |= eventInfo.nominalsOfContinuousStatesChanged;

            if (eventInfo.terminateSimulation) goto TERMINATE_MODEL;
        }

        // enter Continuous-Time Mode
        CHECK_STATUS(M_fmi3EnterContinuousTimeMode(m));

        // retrieve solution at simulation (re)start
        CHECK_STATUS(recordVariables(outputFile, m, time));

        if (initialEventMode || valuesOfContinuousStatesChanged) {
            // the model signals a value change of states, retrieve them
            CHECK_STATUS(M_fmi3GetContinuousStates(m, x, NUMBER_OF_STATES));
        }

        if (initialEventMode || nominalsOfContinuousStatesChanged) {
            // the meaning of states has changed; retrieve new nominal values
            CHECK_STATUS(M_fmi3GetNominalsOfContinuousStates(m, x_nominal, NUMBER_OF_STATES));
        }

        if (eventInfo.nextEventTimeDefined) {
            tNext = min(eventInfo.nextEventTime, tEnd);
        } else {
            tNext = tEnd;
        }

        initialEventMode = fmi3False;
    }

    if (time >= tEnd) {
        goto TERMINATE_MODEL;
    }

    // compute derivatives
    CHECK_STATUS(M_fmi3GetDerivatives(m, der_x, NUMBER_OF_STATES));

    // advance time
    h = min(fixedStep, tNext - time);
    time += h;
    CHECK_STATUS(M_fmi3SetTime(m, time));

    // set continuous inputs at t = time
    // M_fmi3SetFloat*(m, ...)

    // set states at t = time and perform one step
    for (size_t i = 0; i < NUMBER_OF_STATES; i++) {
        x[i] += h * der_x[i]; // forward Euler method
    }

    CHECK_STATUS(M_fmi3SetContinuousStates(m, x, NUMBER_OF_STATES));

    // get event indicators at t = time
    CHECK_STATUS(M_fmi3GetEventIndicators(m, z, NUMBER_OF_EVENT_INDICATORS));

    stateEvent = fmi3False;

    for (size_t i = 0; i < NUMBER_OF_EVENT_INDICATORS; i++) {
        
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
    
TERMINATE_MODEL:

    if (m && status != fmi3Error && status != fmi3Fatal) {
        // retrieve final values and terminate simulation
        CHECK_STATUS(recordVariables(outputFile, m, time));
        status = max(status, M_fmi3Terminate(m));
    }
    
    if (m && status != fmi3Fatal) {
        // clean up
        M_fmi3FreeInstance(m);
    }

    printf("done.\n");

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
