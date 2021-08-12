#include <math.h>

#define OUTPUT_FILE  "cs_event_mode_out.csv"
#define LOG_FILE     "cs_event_mode_log.txt"

#include "util.h"


// Callback
void intermediateUpdate (
    fmi3InstanceEnvironment instanceEnvironment,
    fmi3Float64  intermediateUpdateTime,
    fmi3Boolean  clocksTicked,
    fmi3Boolean  intermediateVariableSetRequested,
    fmi3Boolean  intermediateVariableGetAllowed,
    fmi3Boolean  intermediateStepFinished,
    fmi3Boolean  canReturnEarly,
    fmi3Boolean* earlyReturnRequested,
    fmi3Float64* earlyReturnTime) {

    // ignore clocksTicked
    // ignore intermediate variables (will be handled in Event Mode)
    // ignore intermediateStepFinished

    if (canReturnEarly) {
        // stop at intermediateUpdateTime
        *earlyReturnRequested = fmi3True;
        *earlyReturnTime = intermediateUpdateTime;
    }
}

int main(int argc, char* argv[]) {

    CALL(setUp());

    //////////////////////////
    // Initialization sub-phase

    // Instantiate the FMU
    CALL(FMI3InstantiateCoSimulation(S,
        INSTANTIATION_TOKEN, // instantiationToken
        NULL,                // resourcePath
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        fmi3False,           // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        intermediateUpdate   // intermediateUpdate
    ));

    // Set all variable start values (of "ScalarVariable / <type> / start")
    // fmi3SetReal/Integer/Boolean/String(s, ...);

    // Initialize the FMU instance
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, startTime, fmi3True, stopTime));

    // Set the input values at time = startTime
    // fmi3SetReal/Integer/Boolean/String(s, ...);
    CALL(FMI3ExitInitializationMode(S));

    //////////////////////////
    // Simulation sub-phase
    fmi3Float64 time = startTime; // current time
    fmi3Float64 step = h;         // non-zero step size (0: enter Event Mode)

    while (time < stopTime) {

        if (step > 0) {
            // Continuous mode (default mode)
            fmi3Boolean eventEncountered = fmi3False, terminate = fmi3False, earlyReturn = fmi3False;
            fmi3Float64 lastSuccessfulTime = 0;

            CALL(FMI3DoStep(S, time, step, fmi3False, &eventEncountered, &terminate, &earlyReturn, &lastSuccessfulTime));

            // TODO: use eventEncountered instead of earlyReturn
            if (earlyReturn) {
                // TODO: pass reasons
                CALL(FMI3EnterEventMode(S, fmi3False, fmi3False, NULL, 0, fmi3False));
                step = 0;
            } else {
                step = h;
            }

            // TODO: use lastSuccessfulTime
            time += h;

        } else {

            fmi3Boolean discreteStatesNeedUpdate = fmi3True;
            fmi3Boolean terminateSimulation;
            fmi3Boolean nominalsOfContinuousStatesChanged;
            fmi3Boolean valuesOfContinuousStatesChanged;
            fmi3Boolean nextEventTimeDefined;
            fmi3Float64 nextEventTime;

            // Event mode
            CALL(FMI3UpdateDiscreteStates(S, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsOfContinuousStatesChanged, &valuesOfContinuousStatesChanged, &nextEventTimeDefined, &nextEventTime));

            if (!discreteStatesNeedUpdate) {
                CALL(FMI3EnterStepMode(S));
                step = h - fmod(time, h);  // finish the step
            };
        };

        // Get outputs
        // fmi3GetReal/Integer/Boolean/String(s, ...);
        recordVariables(S, outputFile);

        // Set inputs
        // fmi3SetReal/Integer/Boolean/String(s, ...);
    };

TERMINATE:
    return tearDown();
}
