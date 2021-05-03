#include <math.h>
#include "FMU.h"
#include "util.h"
#include "config.h"

int main(int argc, char* argv[]) {

#if defined(_WIN32)
    const char *sharedLibrary = xstr(MODEL_IDENTIFIER) "\\binaries\\x86_64-windows\\" xstr(MODEL_IDENTIFIER) ".dll";
#elif defined(__APPLE__)
    const char *sharedLibrary = xstr(MODEL_IDENTIFIER) "/binaries/x86_64-darwin/" xstr(MODEL_IDENTIFIER) ".dylib";
#else
    const char *sharedLibrary = xstr(MODEL_IDENTIFIER) "/binaries/x86_64-linux/" xstr(MODEL_IDENTIFIER) ".so";
#endif

    FMU *S = loadFMU(sharedLibrary);

    if (!S) {
        return EXIT_FAILURE;
    }

    fmi3Status status = fmi3OK;
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

    FILE *outputFile = NULL;

    printf("Running " xstr(MODEL_IDENTIFIER) " as Model Exchange... \n");

    outputFile = openOutputFile(xstr(MODEL_IDENTIFIER) "_me.csv");

    if (!outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

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
    fmi3Instance s = S->fmi3InstantiateModelExchange(
        "intstance1",        // instanceName
        INSTANTIATION_TOKEN, // instantiationToken
        NULL,                // resourceLocation
        fmi3False,           // visible
        fmi3False,           // loggingOn
        NULL,                // instanceEnvironment
        cb_logMessage        // logMessage
    );

    if (s == NULL) {
        status = fmi3Error;
        goto TERMINATE;
    }

    // set the start time
    fmi3Float64 time = 0;

    // TODO: set all variable start values (of "ScalarVariable / <type> / start")

    // initialize
    // determine continuous and discrete states
    CHECK_STATUS(S->fmi3EnterInitializationMode(s, fmi3False, 0.0, time, fmi3True, stopTime));

    // TODO: apply input

    CHECK_STATUS(S->fmi3ExitInitializationMode(s));

    // intial event iteration
    while (discreteStatesNeedUpdate) {

        CHECK_STATUS(S->fmi3UpdateDiscreteStates(
            s,
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

    CHECK_STATUS(S->fmi3EnterContinuousTimeMode(s));

#if NZ > 0
    // initialize previous event indicators
    CHECK_STATUS(S->fmi3GetEventIndicators(s, previous_z, NZ));
#endif

#if NX > 0
    // retrieve initial state x and
    // nominal values of x (if absolute tolerance is needed)
    CHECK_STATUS(S->fmi3GetContinuousStates(s, x, NX));
    CHECK_STATUS(S->fmi3GetNominalsOfContinuousStates(s, x_nominal, NX));
#endif

    // retrieve solution at t=Tstart, for example, for outputs
    // S->fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

    while (!terminateSimulation) {

        // detect time event
        timeEvent = nextEventTimeDefined && time >= nextEventTime;

        // handle events
        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            CHECK_STATUS(S->fmi3EnterEventMode(s, stepEvent, stateEvent, rootsFound, NZ, timeEvent));

            nominalsOfContinuousStatesChanged = fmi3False;
            valuesOfContinuousStatesChanged = fmi3False;

            // event iteration
            do {
                // set inputs at super dense time point
                // S->fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;

                // update discrete states
                CHECK_STATUS(S->fmi3UpdateDiscreteStates(s, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

                // get output at super dense time point
                // S->fmi3GetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

                nominalsOfContinuousStatesChanged |= nominalsChanged;
                valuesOfContinuousStatesChanged |= statesChanged;

                if (terminateSimulation) {
                    goto TERMINATE;
                }

            } while (discreteStatesNeedUpdate);

            // enter Continuous-Time Mode
            CHECK_STATUS(S->fmi3EnterContinuousTimeMode(s));

            // retrieve solution at simulation (re)start
            CHECK_STATUS(recordVariables(outputFile, S, s, time));

#if NX > 0
            if (valuesOfContinuousStatesChanged) {
                // the model signals a value change of states, retrieve them
                CHECK_STATUS(S->fmi3GetContinuousStates(s, x, NX));
            }

            if (nominalsOfContinuousStatesChanged) {
                // the meaning of states has changed; retrieve new nominal values
                CHECK_STATUS(S->fmi3GetNominalsOfContinuousStates(s, x_nominal, NX));
            }
#endif
        }

        if (time >= stopTime) {
            goto TERMINATE;
        }

#if NX > 0
        // compute continous state derivatives
        CHECK_STATUS(S->fmi3GetContinuousStateDerivatives(s, der_x, NX));
#endif
        // advance time
        time += fixedStep;

        CHECK_STATUS(S->fmi3SetTime(s, time));

        // set continuous inputs at t = time
        // S->fmi3SetFloat*(m, ...)

#if NX > 0
        // set states at t = time and perform one step
        for (size_t i = 0; i < NX; i++) {
            x[i] += fixedStep * der_x[i]; // forward Euler method
        }

        CHECK_STATUS(S->fmi3SetContinuousStates(s, x, NX));
#endif

#if NZ > 0
        // get event indicators at t = time
        CHECK_STATUS(S->fmi3GetEventIndicators(s, z, NZ));

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
        CHECK_STATUS(S->fmi3CompletedIntegratorStep(s, fmi3True, &stepEvent, &terminateSimulation));

        // get continuous output
        // S->fmi3GetFloat*(m, ...)
        CHECK_STATUS(recordVariables(outputFile, S, s, time));
    }

TERMINATE:

    if (s && status < fmi3Error) {
        // retrieve final values and terminate simulation
        CHECK_STATUS(recordVariables(outputFile, S, s, time));
        const fmi3Status terminateStatus = S->fmi3Terminate(s);
        status = max(status, terminateStatus);
    }

    if (s && status < fmi3Fatal) {
        // clean up
        S->fmi3FreeInstance(s);
    }
    // end::ModelExchange[]

    fclose(outputFile);

    printf("done.\n");

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
