#include <stdlib.h>
#include <string.h>

#include "FMI3.h"
#include "FMISimulation.h"
#include "FMI1CSSimulation.h"
#include "FMI1MESimulation.h"
#include "FMI2CSSimulation.h"
#include "FMI2MESimulation.h"
#include "FMI3CSSimulation.h"
#include "FMI3MESimulation.h"
#include "FMIUtil.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

FMIStatus FMIApplyStartValues(FMIInstance* S, const FMISimulationSettings* settings) {

    FMIStatus status = FMIOK;

    size_t nValues = 0;
    void* values = NULL;
    size_t* sizes = NULL;

    bool configurationMode = false;

    for (size_t i = 0; i < settings->nStartValues; i++) {

        const FMIModelVariable* variable = settings->startVariables[i];
        const FMICausality causality = variable->causality;
        const FMIValueReference vr = variable->valueReference;
        const FMIVariableType type = variable->type;
        const char* literal = settings->startValues[i];

        if (causality == FMIStructuralParameter && type == FMIUInt64Type) {

            CALL(FMIParseValues(FMIMajorVersion3, type, literal, &nValues, &values, NULL));

            if (!configurationMode) {
                CALL(FMI3EnterConfigurationMode(S));
                configurationMode = true;
            }

            CALL(FMI3SetUInt64(S, &vr, 1, (fmi3UInt64*)values, nValues));

            FMIFree(&values);
        }
    }

    if (configurationMode) {
        CALL(FMI3ExitConfigurationMode(S));
    }

    for (size_t i = 0; i < settings->nStartValues; i++) {

        const FMIModelVariable* variable = settings->startVariables[i];
        const FMICausality causality = variable->causality;
        const FMIValueReference vr = variable->valueReference;
        const FMIVariableType type = variable->type;
        const char* literal = settings->startValues[i];

        if (causality == FMIStructuralParameter) {
            continue;
        }

        size_t* sizes = { NULL };

        CALL(FMIParseValues(S->fmiMajorVersion, type, literal, &nValues, &values, &sizes));

        CALL(FMISetValues(S, type, &vr, 1, sizes, values, nValues));

        FMIFree(&values);
    }

TERMINATE:

    FMIFree(&values);

    return status;
}

FMIStatus FMISimulate(const FMISimulationSettings* settings) {

    FMIStatus status = FMIOK;

    const bool cs = settings->interfaceType == FMICoSimulation;

    switch (settings->modelDescription->fmiMajorVersion) {
    case FMIMajorVersion1:
        if (cs) {
            CALL(FMI1CSSimulate(settings));
        } else {
            CALL(FMI1MESimulate(settings));
        }
        break;
    case FMIMajorVersion2:
        if (cs) {
            CALL(FMI2CSSimulate(settings));
        } else {
            CALL(FMI2MESimulate(settings));
        }
        break;
    case FMIMajorVersion3:
        if (cs) {
            CALL(FMI3CSSimulate(settings));
        } else {
            CALL(FMI3MESimulate(settings));
        }
        break;
    }

TERMINATE:
    return status;
}
