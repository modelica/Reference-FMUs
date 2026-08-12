#include <math.h>  // for fabs()
#include <float.h> // for DBL_MIN
#include "config.h"
#include "model.h"

#define V_MIN (0.1)
#define EVENT_EPSILON (1e-10)


Status setStartValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    M(h) =  1;
    M(v) =  0;
    M(g) = -9.81;
    M(e) =  0.7;

    comp->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    comp->isDirtyValues = false;

    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(comp);

    switch (vr) {
        case vr_time:
            ASSERT_NVALUES(1);
            values[(*index)++] = comp->time;
            return OK;
        case vr_h:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(h);
            return OK;
        case vr_der_h:
        case vr_v:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(v);
            return OK;
        case vr_der_v:
        case vr_g:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(g);
            return OK;
        case vr_e:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(e);
            return OK;
        case vr_v_min:
            ASSERT_NVALUES(1);
            values[(*index)++] = V_MIN;
            return OK;
        default:
            logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {

        case vr_h:
            if (comp->state != Instantiated &&
                comp->state != InitializationMode &&
                comp->state != ContinuousTimeMode &&
                comp->state != EventMode) {
                logError(comp, "Variable \"h\" can only be set in Instantiated Mode, Initialization Mode, Continuous Time Mode, and Event Mode.");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(h) = values[(*index)++];
            break;

        case vr_v:
            if (comp->state != Instantiated &&
                comp->state != InitializationMode &&
                comp->state != ContinuousTimeMode &&
                comp->state != EventMode) {
                logError(comp, "Variable \"v\" can only be set in Instantiated Mode, Initialization Mode, Continuous Time Mode, and Event Mode.");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(v) = values[(*index)++];
            break;

        case vr_g:
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode) {
                logError(comp, "Variable g can only be set after instantiation or in initialization mode.");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(g) = values[(*index)++];
            break;

        case vr_e:
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable e can only be set after instantiation, in initialization mode or event mode.");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(e) = values[(*index)++];
            break;

        case vr_v_min:
            logError(comp, "Variable v_min (value reference %u) is constant and cannot be set.", vr_v_min);
            return Error;

        default:
            logError(comp, "Unexpected value reference: %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status getOutputDerivative(ModelInstance *comp, ValueReference valueReference, int order, double *value) {
    ASSERT_NOT_NULL2(comp);
    if (order != 1) {
        logError(comp, "The output derivative order %d for value reference %u is not available.", order, valueReference);
        return Error;
    }
    ASSERT_NOT_NULL2(value);

    calculateValues(comp);

    switch (valueReference) {
    case vr_h:
        *value = M(v);
        return OK;
    case vr_v:
        *value = M(g);
        return OK;
    default:
        logError(comp, "The output derivative for value reference %u is not available.", valueReference);
        return Error;
    }
}

Status eventUpdate(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    if (M(h) <= 0 && M(v) < 0) {

        M(h) = DBL_MIN;  // slightly above 0 to avoid zero-crossing
        M(v) = -M(v) * M(e);

        if (M(v) < V_MIN) {
            // stop bouncing
            M(v) = 0;
            M(g) = 0;
        }

        comp->valuesOfContinuousStatesChanged = true;
    } else {
        comp->valuesOfContinuousStatesChanged = false;
    }

    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation  = false;
    comp->nextEventTimeDefined = false;

    comp->isDirtyValues = true;

    return OK;
}

size_t getNumberOfEventIndicators(ModelInstance* comp) {
    UNUSED(comp);
    return MAX_EVENT_INDICATORS;
}

size_t getNumberOfContinuousStates(ModelInstance* comp) {
    UNUSED(comp);
    return MAX_CONTINUOUS_STATES;
}

Status getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    x[0] = M(h);
    x[1] = M(v);

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance* comp, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(nominals);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    nominals[0] = 1.0;
    nominals[1] = 1.0;

    return OK;
}

Status setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    M(h) = x[0];
    M(v) = x[1];

    comp->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(dx);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    dx[0] = M(v);
    dx[1] = M(g);

    return OK;
}

Status getEventIndicators(ModelInstance *comp, double z[], size_t nz) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(z);
    ASSERT_SIZE_T(nz, MAX_EVENT_INDICATORS);

    calculateValues(comp);

    if (M(h) > -EVENT_EPSILON && M(h) <= 0 && M(v) > 0) {
        // hysteresis for better stability
        z[0] = -EVENT_EPSILON;
    } else {
        z[0] = M(h);
    }

    return OK;
}
