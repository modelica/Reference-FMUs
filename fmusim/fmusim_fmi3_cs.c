/**************************************************************
 *  Copyright (c) Modelica Association Project "FMI".         *
 *  All rights reserved.                                      *
 *  This file is part of the Reference FMUs. See LICENSE.txt  *
 *  in the project root for license information.              *
 **************************************************************/

#include "fmusim_fmi3.h"
#include "fmusim_fmi3_cs.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI3CS(FMIInstance* S,
    const FMIModelDescription * modelDescription,
    const char* resourcePath,
    FMISimulationResult* result,
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[],
    double startTime,
    double stepSize,
    double stopTime,
    const FMUStaticInput* input) {

    FMIStatus status = FMIOK;

    fmi3Boolean eventEncountered = fmi3False;
    fmi3Boolean terminateSimulation = fmi3False;
    fmi3Boolean earlyReturn = fmi3False;
    fmi3Float64 lastSuccessfulTime = startTime;
    fmi3Float64 time = startTime;

    CALL(FMI3InstantiateCoSimulation(S,
        modelDescription->instantiationToken,  // instantiationToken
        resourcePath,                          // resourcePath
        fmi3False,                             // visible
        fmi3False,                             // loggingOn
        fmi3False,                             // eventModeUsed
        fmi3False,                             // earlyReturnAllowed
        NULL,                                  // requiredIntermediateVariables
        0,                                     // nRequiredIntermediateVariables
        NULL                                   // intermediateUpdate
    ));

    CALL(applyStartValuesFMI3(S, nStartValues, startVariables, startValues));
    CALL(FMIApplyInput(S, input, startTime, true, true, false));

    // initialize
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, startTime, fmi3True, stopTime));
    CALL(FMI3ExitInitializationMode(S));

    size_t step = 0;

    for (;; step++) {

        const fmi3Float64 time = startTime + step * stepSize;

        CALL(FMISample(S, time, result));

        if ((step + 1) * stepSize > stopTime) {
            break;
        }

        CALL(FMIApplyInput(S, input, time, true, true, false));

        CALL(FMI3DoStep(S, time, stepSize, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime));

        if (terminateSimulation) {
            break;
        }
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