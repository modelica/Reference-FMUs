#include <stdlib.h>
#include "fmusim_fmi2.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus applyStartValuesFMI2(
    FMIInstance* S,
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[]) {

    FMIStatus status = FMIOK;

    for (size_t i = 0; i < nStartValues; i++) {

        const FMIModelVariable* variable = startVariables[i];
        const FMIValueReference vr = variable->valueReference;
        const char* literal = startValues[i];

        switch (variable->type) {
        case FMIRealType: {
            const fmi2Real value = strtod(literal, NULL);
            // TODO: handle errors
            CALL(FMI2SetReal(S, &vr, 1, &value));
            break;
        }
        }
    }

TERMINATE:
    return status;
}
