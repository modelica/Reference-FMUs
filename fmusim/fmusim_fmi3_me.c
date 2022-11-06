#include "fmusim_fmi3_me.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI3ME(FMIInstance* S, const char* instantiationToken, const char* resourcePath,
    FMISimulationResult* result,
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[],
    double startTime,
    double stepSize,
    double stopTime) {

    return FMIFatal;

}