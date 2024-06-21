#include "FMI3.h"
#include "FMIUtil.h"
#include "FMISimulation.h"
#include "stdlib.h"

#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus FMIApplyStartValues(FMIInstance* S, const FMISimulationSettings* settings) {

    FMIStatus status = FMIOK;

    size_t nValues = 0;
    void* values = NULL;

    bool configurationMode = false;

    for (size_t i = 0; i < settings->nStartValues; i++) {

        const FMIModelVariable* variable = settings->startVariables[i];
        const FMICausality causality = variable->causality;
        const FMIValueReference vr = variable->valueReference;
        const FMIVariableType type = variable->type;
        const char* literal = settings->startValues[i];

        if (causality == FMIStructuralParameter && type == FMIUInt64Type) {

            CALL(FMIParseValues(FMIMajorVersion3, type, literal, &nValues, &values));

            if (!configurationMode) {
                CALL(FMI3EnterConfigurationMode(S));
                configurationMode = true;
            }

            CALL(FMI3SetUInt64(S, &vr, 1, (fmi3UInt64*)values, nValues));

            free(values);
            values = NULL;
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

        CALL(FMIParseValues(S->fmiMajorVersion, type, literal, &nValues, &values));


        if (variable->type == FMIBinaryType) {

            const size_t size = strlen(literal) / 2;
            CALL(FMI3SetBinary(S, &vr, 1, &size, values, 1));

        } else {

            if (S->fmiMajorVersion == FMIMajorVersion1) {
                CALL(FMI1SetValues(S, type, &vr, 1, values));
            } else if (S->fmiMajorVersion == FMIMajorVersion2) {
                CALL(FMI2SetValues(S, type, &vr, 1, values));
            } else if (S->fmiMajorVersion == FMIMajorVersion3) {
                CALL(FMI3SetValues(S, type, &vr, 1, values, nValues));
            }
        }

        free(values);
        values = NULL;
    }

TERMINATE:
    if (values) {
        free(values);
    }

    return status;
}
