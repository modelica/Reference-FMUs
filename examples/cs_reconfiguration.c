#include <math.h>

#define OUTPUT_FILE  "cs_reconfiguration_out.csv"
#define LOG_FILE     "cs_reconfiguration_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    // communication step size
    const fmi3Float64 stepSize = 0.1;

    fmi3Float64 time = startTime;

    fmi3Boolean eventEncountered, terminateSimulation, earlyReturn;

    fmi3ValueReference vr[3];
    fmi3Float64 D[R_MAX * M_MAX];
    fmi3Float64 u[M_MAX];
    fmi3UInt64 p[3];

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

    CALL(FMI3EnterConfigurationMode(S));
    
    vr[0] = vr_m; p[0] = 2; // number of inputs
    vr[1] = vr_n; p[1] = 0; // number of states
    vr[2] = vr_r; p[2] = 2; // number of outputs
     
    CALL(FMI3SetUInt64(S, vr, 3, &p, 3));

    CALL(FMI3ExitConfigurationMode(S));

    vr[0] = vr_D;

    // 2x2 identity matrix
    D[0] = 1; D[1] = 0;
    D[2] = 0; D[3] = 1;

    CALL(FMI3SetFloat64(S, vr, 1, D, 4));

    // intial inputs
    vr[0] = vr_u;
    u[0] = 0.0;
    u[1] = 1.0;
    CALL(FMI3SetFloat64(S, vr, 1, u, 2));

    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, stopTime));
    CALL(FMI3ExitInitializationMode(S));

    for (size_t step = 0; step <= 10; step++) {

        CALL(recordVariables(S, outputFile));

        if (step == 5) {
            CALL(FMI3EnterConfigurationMode(S));

            vr[0] = vr_m; vr[1] = vr_r;
            p[0] = 3; p[1] = 3;

            CALL(FMI3SetUInt64(S, vr, 2, p, 2));

            CALL(FMI3ExitConfigurationMode(S));

            vr[0] = vr_D;

            // 3x3 identity matrix
            D[0] = 1; D[1] = 0; D[2] = 0;
            D[3] = 0; D[4] = 1; D[5] = 0;
            D[6] = 0; D[7] = 0; D[8] = 1;

            CALL(FMI3SetFloat64(S, vr, 1, D, 9));
        }

        for (size_t i = 0; i < M_MAX; i++) {
            u[i] = time + stepSize + i;
        }

        vr[0] = vr_u;
        CALL(FMI3SetFloat64(S, vr, 1, u, step < 5 ? 2 : 3));
        
        CALL(FMI3DoStep(S,
            time,                 // currentCommunicationPoint
            stepSize,             // communicationStepSize
            fmi3True,             // noSetFMUStatePriorToCurrentPoint
            &eventEncountered,    // eventEncountered
            &terminateSimulation, // terminate
            &earlyReturn,         // earlyReturn
            &time                 // lastSuccessfulTime
        ));
    }

TERMINATE:
    return tearDown();
}
