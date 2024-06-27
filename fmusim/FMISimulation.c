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

#define FMI_PATH_MAX 4096

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

FMIStatus FMISimulate(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    const char* unzipdir,
    FMIRecorder* recorder,
    const FMIStaticInput* input,
    const FMISimulationSettings* settings) {

    FMIStatus status = FMIOK;

    char resourcePath[FMI_PATH_MAX] = "";

#ifdef _WIN32
    snprintf(resourcePath, FMI_PATH_MAX, "%s\\resources\\", unzipdir);
#else
    snprintf(resourcePath, FMI_PATH_MAX, "%s/resources/", unzipdir);
#endif

    if (modelDescription->fmiMajorVersion == FMIMajorVersion1) {

        if (settings->interfaceType == FMICoSimulation) {
            char fmuLocation[FMI_PATH_MAX] = "";
            CALL(FMIPathToURI(unzipdir, fmuLocation, FMI_PATH_MAX));
            CALL(FMI1CSSimulate(S, modelDescription, fmuLocation, recorder, input, settings));
        } else {
            CALL(FMI1MESimulate(S, modelDescription, recorder, input, settings));
        }

    } else if (modelDescription->fmiMajorVersion == FMIMajorVersion2) {

        char resourceURI[FMI_PATH_MAX] = "";
        CALL(FMIPathToURI(resourcePath, resourceURI, FMI_PATH_MAX));

        if (settings->interfaceType == FMICoSimulation) {
            CALL(FMI2CSSimulate(S, modelDescription, resourceURI, recorder, input, settings));
        } else {
            CALL(FMI2MESimulate(S, modelDescription, resourceURI, recorder, input, settings));
        }

    } else {

        if (settings->interfaceType == FMICoSimulation) {
            CALL(FMI3CSSimulate(S, modelDescription, resourcePath, recorder, input, settings));
        } else {
            CALL(FMI3MESimulate(S, modelDescription, resourcePath, recorder, input, settings));
        }

    }

TERMINATE:
    return status;
}
