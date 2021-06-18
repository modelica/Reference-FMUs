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
    M(x) =  0.0;
    M(der_x) =  0.0;
    M(u) = 0.0;
}

void calculateValues(ModelInstance *comp) {
    M(der_x) = - M(x) + M(u);
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    switch (vr) {
        case vr_x:
            value[(*index)++] = M(x);
            return OK;
        case vr_der_x:
            value[(*index)++] = M(der_x);
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {
        case vr_u:
            M(u) = value[(*index)++];
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

void getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    x[0] = M(x);
}

void setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    M(x) = x[0];
}

void getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    dx[0] = M(der_x);
}
