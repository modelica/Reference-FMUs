#include <math.h>

#define OUTPUT_FILE  "cs_reconfiguration_out.csv"
#define LOG_FILE     "cs_reconfiguration_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

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
        NULL                 // intermediateUpdate
    ));

    // set start values
    CALL(applyStartValues(S));

    fmi3Float64 time = startTime;

    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, stopTime));
    CALL(FMI3ExitInitializationMode(S));

    const FMIValueReference vr_structural_parameters[3] = { vr_m, vr_n, vr_r };
    
    fmi3UInt64 structural_parameters[3] = { 2, 2, 2 };

    // communication step size
    const fmi3Float64 stepSize = 10 * FIXED_SOLVER_STEP;

    for (size_t step = 0; step <= 10; step++) {

        fmi3Boolean eventEncountered, terminateSimulation, earlyReturn;

        if (step == 5) {
            CALL(FMI3EnterConfigurationMode(S));
            CALL(FMI3SetUInt64(S, vr_structural_parameters, 3, structural_parameters, 3));
            CALL(FMI3ExitConfigurationMode(S));
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

        time = startTime + step * stepSize;
    };

TERMINATE:
    return tearDown();
}
