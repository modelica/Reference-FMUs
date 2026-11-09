#include <math.h>  // for fabs()
#include <float.h> // for DBL_MIN
#include "config.h"
#include "model_instance.h"

#define V_MIN (0.1)
#define E_MIN (0.5)
#define E_MAX (1.0)
#define EVENT_EPSILON (1e-10)


Status setStartValues(ModelInstance *instance) {
    ASSERT_NOT_NULL2(instance);

    M(h) =  1;
    M(v) =  0;
    M(g) = -9.81;
    M(e) =  0.7;

    instance->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {
    ASSERT_NOT_NULL2(instance);

    instance->isDirtyValues = false;

    return OK;
}

Status getVariableSize(ModelInstance *instance, ValueReference vr, size_t *size) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(size);

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
        return Error;
    }
}

Status getFloat64(ModelInstance *instance, ValueReference vr, double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_time:
            ASSERT_NVALUES(1);
            values[(*index)++] = instance->time;
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
            logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance *instance, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {

        case vr_h:
            if (instance->state != Instantiated &&
                instance->state != InitializationMode &&
                instance->state != ContinuousTimeMode &&
                instance->state != EventMode) {
                logError(instance, "Variable \"h\" can only be set in Instantiated Mode, Initialization Mode, Continuous Time Mode, and Event Mode.");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(h) = values[(*index)++];
            break;

        case vr_v:
            if (instance->state != Instantiated &&
                instance->state != InitializationMode &&
                instance->state != ContinuousTimeMode &&
                instance->state != EventMode) {
                logError(instance, "Variable \"v\" can only be set in Instantiated Mode, Initialization Mode, Continuous Time Mode, and Event Mode.");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(v) = values[(*index)++];
            break;

        case vr_g:
            if (instance->type == ModelExchange &&
                instance->state != Instantiated &&
                instance->state != InitializationMode) {
                logError(instance, "Variable g can only be set after instantiation or in initialization mode.");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(g) = values[(*index)++];
            break;

        case vr_e:
            if (instance->type == ModelExchange &&
                instance->state != Instantiated &&
                instance->state != InitializationMode &&
                instance->state != EventMode) {
                logError(instance, "Variable e can only be set after instantiation, in initialization mode or event mode.");
                return Error;
            }
            ASSERT_NVALUES(1);
            if (values[*index] < E_MIN || values[*index] > E_MAX) {
                logError(instance, "Variable \"e\" must be in the range [%g, %g] but was %g.",
                    E_MIN, E_MAX, values[*index]);
                return Error;
            }
            M(e) = values[(*index)++];
            break;

        case vr_v_min:
            logError(instance, "Variable v_min (value reference %u) is constant and cannot be set.", vr_v_min);
            return Error;

        default:
            logError(instance, "Unexpected value reference: %u.", vr);
            return Error;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getOutputDerivative(ModelInstance *instance, ValueReference valueReference, int order, double *value) {
    ASSERT_NOT_NULL2(instance);
    if (order != 1) {
        logError(instance, "The output derivative order %d for value reference %u is not available.", order, valueReference);
        return Error;
    }
    ASSERT_NOT_NULL2(value);

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
        return Error;
    }
}

Status updateDiscreteStates(
    ModelInstance *instance,
    bool* discreteStatesNeedUpdate,
    bool* terminateSimulation,
    bool* nominalsOfContinuousStatesChanged,
    bool* valuesOfContinuousStatesChanged,
    bool* nextEventTimeDefined,
    double* nextEventTime
) {
    if (M(h) <= 0 && M(v) < 0) {

        M(h) = DBL_MIN;  // slightly above 0 to avoid zero-crossing
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
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(instance);

    x[0] = M(h);
    x[1] = M(v);

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance *instance, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(nominals);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(instance);

    nominals[0] = 1.0;
    nominals[1] = 1.0;

    return OK;
}

Status setContinuousStates(ModelInstance *instance, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    M(h) = x[0];
    M(v) = x[1];

    instance->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *instance, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(dx);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(instance);

    dx[0] = M(v);
    dx[1] = M(g);

    return OK;
}

Status getEventIndicators(ModelInstance *instance, double z[], size_t nz) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(z);
    ASSERT_SIZE_T(nz, MAX_EVENT_INDICATORS);

    calculateValues(instance);

    if (M(h) > -EVENT_EPSILON && M(h) <= 0 && M(v) > 0) {
        // hysteresis for better stability
        z[0] = -EVENT_EPSILON;
    } else {
        z[0] = M(h);
    }

    return OK;
}
