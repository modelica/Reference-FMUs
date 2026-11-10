#include <math.h>  // for fabs()
#include <float.h> // for DBL_MIN
#include "config.h"
#include "model_impl.h"

#define V_MIN (0.1)
#define E_MIN (0.5)
#define E_MAX (1.0)
#define EVENT_EPSILON (1e-10)

Status setStartValues(ModelInstance *instance) {
    ASSERT_NOT_NULL(instance);

    M(h) = 1;
    M(v) = 0;
    M(g) = -9.81;
    M(e) = 0.7;

    instance->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {
    ASSERT_NOT_NULL(instance);

    instance->isDirtyValues = false;

    return OK;
}

Status getVariableSize(ModelInstance *instance, ValueReference vr, size_t *size) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(size);

    switch (vr) {
    case vr_time:
    case vr_h:
    case vr_der_h:
    case vr_v:
    case vr_der_v:
    case vr_g:
    case vr_e:
    case vr_v_min:
        *size = 1;
        return OK;
    default:
        logError(instance, "Unknown value reference: %u.", vr);
        return Discard;
    }
}

Status getFloat64(ModelInstance *instance, ValueReference vr, double values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_time:
        values[0] = instance->time;
        return OK;
    case vr_h:
        values[0] = M(h);
        return OK;
    case vr_der_h:
    case vr_v:
        values[0] = M(v);
        return OK;
    case vr_der_v:
    case vr_g:
        values[0] = M(g);
        return OK;
    case vr_e:
        values[0] = M(e);
        return OK;
    case vr_v_min:
        values[0] = V_MIN;
        return OK;
    default:
        logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status setFloat64(ModelInstance *instance, ValueReference vr, const double values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_h:
        if (instance->state != Instantiated &&
            instance->state != InitializationMode &&
            instance->state != ContinuousTimeMode &&
            instance->state != EventMode) {
            logError(instance, "Variable \"h\" can only be set in Instantiated Mode, Initialization Mode, Continuous Time Mode, and Event Mode.");
            return Discard;
        }
        M(h) = values[0];
        break;
    case vr_v:
        if (instance->state != Instantiated &&
            instance->state != InitializationMode &&
            instance->state != ContinuousTimeMode &&
            instance->state != EventMode) {
            logError(instance, "Variable \"v\" can only be set in Instantiated Mode, Initialization Mode, Continuous Time Mode, and Event Mode.");
            return Discard;
        }
        M(v) = values[0];
        break;
    case vr_g:
        if (instance->type == ModelExchange &&
            instance->state != Instantiated &&
            instance->state != InitializationMode) {
            logError(instance, "Variable g can only be set after instantiation or in initialization mode.");
            return Discard;
        }
        M(g) = values[0];
        break;
    case vr_e:
        if (instance->type == ModelExchange &&
            instance->state != Instantiated &&
            instance->state != InitializationMode &&
            instance->state != EventMode) {
            logError(instance, "Variable e can only be set after instantiation, in initialization mode or event mode.");
            return Discard;
        }
        if (values[0] < E_MIN || values[0] > E_MAX) {
            logError(instance, "Variable \"e\" must be in the range [%g, %g] but was %g.",
                     E_MIN, E_MAX, values[0]);
            return Discard;
        }
        M(e) = values[0];
        break;
    case vr_v_min:
        logError(instance, "Variable v_min (value reference %u) is constant and cannot be set.", vr_v_min);
        return Discard;
    default:
        logError(instance, "Unexpected value reference: %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getOutputDerivative(ModelInstance *instance, ValueReference valueReference, int order, double *value) {
    ASSERT_NOT_NULL(instance);
    if (order != 1) {
        logError(instance, "The output derivative order %d for value reference %u is not available.", order, valueReference);
        return Discard;
    }
    ASSERT_NOT_NULL(value);

    calculateValues(instance);

    switch (valueReference) {
    case vr_h:
        *value = M(v);
        return OK;
    case vr_v:
        *value = M(g);
        return OK;
    default:
        logError(instance, "The output derivative for value reference %u is not available.", valueReference);
        return Discard;
    }
}

Status updateDiscreteStates(
    ModelInstance *instance,
    bool *discreteStatesNeedUpdate,
    bool *terminateSimulation,
    bool *nominalsOfContinuousStatesChanged,
    bool *valuesOfContinuousStatesChanged,
    bool *nextEventTimeDefined,
    double *nextEventTime) {
    if (M(h) <= 0 && M(v) < 0) {

        M(h) = DBL_MIN; // slightly above 0 to avoid zero-crossing
        M(v) = -M(v) * M(e);

        if (M(v) < V_MIN) {
            // stop bouncing
            M(v) = 0;
            M(g) = 0;
        }

        instance->isDirtyValues = true;

        *valuesOfContinuousStatesChanged = true;
    } else {
        *valuesOfContinuousStatesChanged = false;
    }

    *discreteStatesNeedUpdate = false;
    *terminateSimulation = false;
    *nominalsOfContinuousStatesChanged = false;
    *nextEventTimeDefined = false;
    *nextEventTime = 0.0;

    return OK;
}

Status getContinuousStates(ModelInstance *instance, double x[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(x);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    calculateValues(instance);

    x[0] = M(h);
    x[1] = M(v);

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance *instance, double nominals[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(nominals);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    calculateValues(instance);

    nominals[0] = 1.0;
    nominals[1] = 1.0;

    return OK;
}

Status setContinuousStates(ModelInstance *instance, const double x[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(x);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    M(h) = x[0];
    M(v) = x[1];

    instance->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *instance, double dx[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(dx);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    calculateValues(instance);

    dx[0] = M(v);
    dx[1] = M(g);

    return OK;
}

Status getEventIndicators(ModelInstance *instance, double z[], size_t nz) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(z);
    ASSERT_SIZE_T(nz, N_EVENT_INDICATORS);

    calculateValues(instance);

    if (M(h) > -EVENT_EPSILON && M(h) <= 0 && M(v) > 0) {
        // hysteresis for better stability
        z[0] = -EVENT_EPSILON;
    } else {
        z[0] = M(h);
    }

    return OK;
}
