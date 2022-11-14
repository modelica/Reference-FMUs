/**************************************************************
 *  Copyright (c) Modelica Association Project "FMI".         *
 *  All rights reserved.                                      *
 *  This file is part of the Reference FMUs. See LICENSE.txt  *
 *  in the project root for license information.              *
 **************************************************************/

#include <stdlib.h>
#include "fmusim_fmi3.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus applyStartValuesFMI3(
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
        case FMIFloat64Type: {
            const fmi3Float64 value = strtod(literal, NULL);
            // TODO: handle errors
            CALL(FMI3SetFloat64(S, &vr, 1, &value, 1));
            break;
        }
        }
    }

TERMINATE:
    return status;
}