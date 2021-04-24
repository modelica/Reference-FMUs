#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#define FMI3_FUNCTION_PREFIX Plant_
#include "fmi3Functions.h"
#undef FMI3_FUNCTION_PREFIX

#include "util.h"


#define INSTANTIATION_TOKEN "{00000000-0000-0000-0000-000000000000}"
#define Plant_NX 1
#define Plant_NZ 0
#define Plant_Token 1

#define FIXED_STEP 1e-2
#define STOP_TIME 3
#define OUTPUT_FILE_HEADER "time,x\n"


fmi3Status recordVariables(FILE* outputFile, fmi3Instance plant, fmi3Float64 time) {
    const fmi3ValueReference valueReferences[Plant_NX] = { 1 };
    fmi3Float64 values[Plant_NX] = { 0 };
    fmi3Status status = Plant_fmi3GetFloat64(plant, valueReferences, 2, values, 2);
    fprintf(outputFile, "%g,%g\n", time, values[0]);
    return status;
}


int main(int argc, char *argv[])
{
    printf("Running Supervisory Control example... \n");
    
    fmi3Status status = fmi3OK;
    const fmi3Float64 fixedStep = FIXED_STEP;
    fmi3Float64 h = fixedStep;
    fmi3Float64 tNext = h;
    const fmi3Float64 tEnd = STOP_TIME;
    fmi3Float64 time = 0;
    const fmi3Float64 tStart = 0;
    fmi3Boolean timeEvent, stateEvent, enterEventMode, terminateSimulation = fmi3False, initialEventMode;
    fmi3Instance m = NULL;

    fmi3Int32 rootsFound[1] = { 0 };
    fmi3Float64 x[Plant_NX] = { 0 };
    fmi3Float64 der_x[Plant_NX] = { 0 };
    FILE* outputFile = NULL;

    puts("Running model_exchange example...");

    outputFile = fopen("synchronous_control_me_out.csv", "w");

    if (!outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    fputs(OUTPUT_FILE_HEADER, outputFile);

    m = Plant_fmi3InstantiateModelExchange("m", INSTANTIATION_TOKEN, NULL, fmi3False, fmi3True, NULL, cb_logMessage);

    if (m == NULL) {
        status = fmi3Error;
        goto TERMINATE;
    }

    time = tStart;

    CHECK_STATUS(Plant_fmi3EnterInitializationMode(m, fmi3False, 0.0, tStart, fmi3True, tEnd));
    CHECK_STATUS(Plant_fmi3ExitInitializationMode(m));

    initialEventMode = fmi3True;
    enterEventMode = fmi3False;
    timeEvent = fmi3False;
    stateEvent = fmi3False;

    // initialize previous event indicators
    initialEventMode = fmi3False;

    CHECK_STATUS(Plant_fmi3EnterContinuousTimeMode(m));

    // retrieve initial state x and
    // nominal values of x (if absolute tolerance is needed)
    CHECK_STATUS(Plant_fmi3GetContinuousStates(m, x, Plant_NX));
    
    // retrieve solution at t=Tstart, for example, for outputs
    // Plant_fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

    while (!terminateSimulation) {

        tNext = time + h;

        // handle events
        if (enterEventMode || stateEvent || timeEvent) {

            if (!initialEventMode) {
                CHECK_STATUS(Plant_fmi3EnterEventMode(m, enterEventMode, stateEvent, rootsFound, Plant_NZ, timeEvent));
            }

            // event iteration
            fmi3Boolean discreteStatesNeedUpdate = fmi3True;
            fmi3Boolean terminateSimulation = fmi3False;
            fmi3Boolean nominalsOfContinuousStatesChanged = fmi3False;
            fmi3Boolean valuesOfContinuousStatesChanged = fmi3False;
            fmi3Boolean nextEventTimeDefined = fmi3False;
            fmi3Float64 nextEventTime = 0;

            while (discreteStatesNeedUpdate) {

                // set inputs at super dense time point
                // Plant_fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;

                // update discrete states
                CHECK_STATUS(Plant_fmi3UpdateDiscreteStates(m, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

                // getOutput at super dense time point
                // Plant_fmi3GetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

                nominalsOfContinuousStatesChanged |= nominalsChanged;
                valuesOfContinuousStatesChanged |= statesChanged;

                if (terminateSimulation) goto TERMINATE;
            }

            // enter Continuous-Time Mode
            CHECK_STATUS(Plant_fmi3EnterContinuousTimeMode(m));

            // retrieve solution at simulation (re)start
            CHECK_STATUS(recordVariables(outputFile, m, time));

            if (initialEventMode || valuesOfContinuousStatesChanged) {
                // the model signals a value change of states, retrieve them
                CHECK_STATUS(Plant_fmi3GetContinuousStates(m, x, Plant_NX));
            }
            
            if (nextEventTimeDefined) {
                tNext = min(nextEventTime, tEnd);
            }
            else {
                tNext = tEnd;
            }

            initialEventMode = fmi3False;
        }

        if (time >= tEnd) {
            goto TERMINATE;
        }

        // compute derivatives
        CHECK_STATUS(Plant_fmi3GetContinuousStateDerivatives(m, der_x, Plant_NX));

        // advance time
        h = min(fixedStep, tNext - time);
        time += h;
        CHECK_STATUS(Plant_fmi3SetTime(m, time));

        // set continuous inputs at t = time
        // Plant_fmi3SetFloat*(m, ...)

        // set states at t = time and perform one step
        for (size_t i = 0; i < Plant_NX; i++) {
            x[i] += h * der_x[i]; // forward Euler method
        }

        CHECK_STATUS(Plant_fmi3SetContinuousStates(m, x, Plant_NX));

        // inform the model about an accepted step
        CHECK_STATUS(Plant_fmi3CompletedIntegratorStep(m, fmi3True, &enterEventMode, &terminateSimulation));

        // get continuous output
        // Plant_fmi3GetFloat*(m, ...)
        CHECK_STATUS(recordVariables(outputFile, m, time));
    }

TERMINATE:

    if (m && status != fmi3Error && status != fmi3Fatal) {
        // retrieve final values and terminate simulation
        CHECK_STATUS(recordVariables(outputFile, m, time));
        fmi3Status s = Plant_fmi3Terminate(m);
        status = max(status, s);
    }

    if (m && status != fmi3Fatal) {
        // clean up
        Plant_fmi3FreeInstance(m);
    }
    
    printf("Success! \n");
    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
