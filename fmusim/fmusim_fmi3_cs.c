#include "fmusim_fmi3_cs.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI3CS(FMIInstance* S, const char* instantiationToken, const char* resourcePath,
    FMISimulationResult* result,
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[],
    double startTime,
    double stepSize,
    double stopTime,
    bool earlyReturnAllowed) {

    FMIStatus status = FMIOK;

    CALL(FMI3InstantiateCoSimulation(S,
        instantiationToken,  // instantiationToken
        resourcePath,        // resourcePath
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        earlyReturnAllowed,  // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        NULL                 // intermediateUpdate
    ));

    for (size_t i = 0; i < nStartValues; i++) {

        const FMIModelVariable* variable = startVariables[i];
        const FMIValueReference vr = variable->valueReference;
        const char* literal = startValues[i];

        switch (variable->type) {
        case FMIFloat64Type: {
            const fmi3Float64 value = strtod(literal, NULL);
            // TODO: handle errors
            CALL(FMI3SetFloat64(S, &vr, 1, &value, 1));
            break;
        }
        }
    }

    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, startTime, fmi3True, stopTime));

    CALL(FMI3ExitInitializationMode(S));


    fmi3Boolean eventEncountered = fmi3False;
    fmi3Boolean terminateSimulation = fmi3False;
    fmi3Boolean earlyReturn = fmi3False;
    fmi3Float64 lastSuccessfulTime = startTime;

    while (lastSuccessfulTime <= stopTime) {

        CALL(FMISample(S, lastSuccessfulTime, result));

        if (terminateSimulation) {
            break;
        }

        CALL(FMI3DoStep(S, lastSuccessfulTime, stepSize, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime));

    }

TERMINATE:

    if (status != FMIFatal) {

        const FMIStatus terminateStatus = FMI3Terminate(S);

        if (terminateStatus != FMIFatal) {
            FMI3FreeInstance(S);
        }
    }

    return status;
}