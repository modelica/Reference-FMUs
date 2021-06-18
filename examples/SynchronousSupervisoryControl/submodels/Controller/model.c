#include <math.h>  // for fabs()
#include "config.h"
#include "model.h"

// C-code FMUs have functions names prefixed with MODEL_IDENTIFIER_.
// Define DISABLE_PREFIX to build a binary FMU.
#if !defined(DISABLE_PREFIX) && !defined(FMI3_FUNCTION_PREFIX)
#define pasteA(a,b)          a ## b
#define pasteB(a,b)          pasteA(a,b)
#define FMI3_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)
#define fmi3FullName(name) pasteB(FMI3_FUNCTION_PREFIX, name)
#else
  #define fmi3FullName(name) name
#endif

#define setStartValues   fmi3FullName(setStartValues)
#define calculateValues   fmi3FullName(calculateValues)
#define getFloat64   fmi3FullName(getFloat64)
#define setFloat64   fmi3FullName(setFloat64)
#define getContinuousStates   fmi3FullName(getContinuousStates)
#define setContinuousStates   fmi3FullName(setContinuousStates)
#define getDerivatives   fmi3FullName(getDerivatives)
#define getPartialDerivative   fmi3FullName(getPartialDerivative)
#define eventUpdate   fmi3FullName(eventUpdate)
#define logError   fmi3FullName(logError)


void logError(ModelInstance *comp, const char *message, ...);


void setStartValues(ModelInstance *comp) {
    M(r) = false;       // Clock
    M(xr) = 0.0;                    // Sample
    M(ur) = 0.0;                    // Discrete state/output
    M(pre_ur) = 0.0;                // Previous ur
    M(ar) = 0.0;                    // Local var
}

void calculateValues(ModelInstance *comp) {
    UNUSED(comp)
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    switch (vr) {
        case vr_xr:
            value[(*index)++] = M(xr);
            return OK;
        case vr_ur:
            value[(*index)++] = M(ur);
            return OK;
        case vr_pre_ur:
            value[(*index)++] = M(pre_ur);
            return OK;
        case vr_ar:
            value[(*index)++] = M(ar);
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {
        case vr_xr:
            M(xr) = value[(*index)++];
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

void eventUpdate(ModelInstance *comp) {
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation  = false;
    comp->nextEventTimeDefined = false;
}
