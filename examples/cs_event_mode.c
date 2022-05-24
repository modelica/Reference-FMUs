#define OUTPUT_FILE  "cs_event_mode_out.csv"
#define LOG_FILE     "cs_event_mode_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    fmi3Boolean discreteStatesNeedUpdate = fmi3False;
    fmi3Boolean nominalsChanged          = fmi3False;
    fmi3Boolean statesChanged            = fmi3False;
    fmi3Boolean nextEventTimeDefined     = fmi3False;
    fmi3Float64 nextEventTime            = 0;

    CALL(setUp());

    // tag::EventMode[]
    CALL(FMI3InstantiateCoSimulation(S,
        INSTANTIATION_TOKEN, // instantiationToken
        NULL,                // resourcePath
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3True,            // eventModeUsed
        fmi3True,            // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        NULL                 // intermediateUpdate
    ));

    // set start values
    CALL(applyStartValues(S));

    fmi3Float64 time = startTime;

    // initialize the FMU
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, stopTime));

    // apply continuous and discrete inputs
    CALL(applyContinuousInputs(S, true));
    CALL(applyDiscreteInputs(S));

    CALL(FMI3ExitInitializationMode(S));

    // update discrete states
    do {
        CALL(FMI3UpdateDiscreteStates(S,
            &discreteStatesNeedUpdate,
            &terminateSimulation,
            &nominalsChanged,
            &statesChanged,
            &nextEventTimeDefined,
            &nextEventTime
        ));

        if (terminateSimulation) {
            goto TERMINATE;
        }
    } while (discreteStatesNeedUpdate);

    CALL(FMI3EnterStepMode(S));

    // communication step size
    const fmi3Float64 stepSize = 10 * FIXED_SOLVER_STEP;

    while (true) {

        CALL(recordVariables(S, outputFile));

        if (terminateSimulation || time >= stopTime) {
            break;
        }

        CALL(FMI3DoStep(S,
            time,                 // currentCommunicationPoint
            stepSize,             // communicationStepSize
            fmi3True,             // noSetFMUStatePriorToCurrentPoint
            &eventEncountered,    // eventEncountered
            &terminateSimulation, // terminate
            &earlyReturn,         // earlyReturn
            &time                 // lastSuccessfulTime
        ));

        if (eventEncountered) {

            // record variables before event update
            CALL(recordVariables(S, outputFile));

            // enter Event Mode
            CALL(FMI3EnterEventMode(S));

            // apply continuous and discrete inputs
            CALL(applyContinuousInputs(S, true));
            CALL(applyDiscreteInputs(S));

            // update discrete states
            do {
                CALL(FMI3UpdateDiscreteStates(S,
                    &discreteStatesNeedUpdate,
                    &terminateSimulation,
                    &nominalsChanged,
                    &statesChanged,
                    &nextEventTimeDefined,
                    &nextEventTime
                ));

                if (terminateSimulation) {
                    break;
                }
            } while (discreteStatesNeedUpdate);

            // return to Step Mode
            CALL(FMI3EnterStepMode(S));
        }
    }

TERMINATE:
    return tearDown();
    // end::EventMode[]
}
