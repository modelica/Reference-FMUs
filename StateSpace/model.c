#include "config.h"
#include "model.h"


Status setStartValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    M(m) = 3;
    M(n) = 3;
    M(r) = 3;

    // identity matrix
    for (int i = 0; i < M_MAX; i++)
    for (int j = 0; j < N_MAX; j++) {
        M(A)[i][j] = i == j ? 1 : 0;
        M(B)[i][j] = i == j ? 1 : 0;
        M(C)[i][j] = i == j ? 1 : 0;
        M(D)[i][j] = i == j ? 1 : 0;
    }

    for (int i = 0; i < M_MAX; i++) {
        M(u)[i] = i + 1;
    }

    for (int i = 0; i < N_MAX; i++) {
        M(y)[i] = 0;
    }

    for (int i = 0; i < N_MAX; i++) {
        M(x)[i] = M(x0)[i];
        M(x)[i] = 0;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    // der(x) = Ax + Bu
    for (size_t i = 0; i < M(n); i++) {

        M(der_x)[i] = 0;

        for (size_t j = 0; j < M(n); j++) {
            M(der_x)[i] += M(A)[i][j] * M(x)[j];
        }
    }

    for (size_t i = 0; i < M(n); i++) {

        for (size_t j = 0; j < M(m); j++) {
            M(der_x)[i] += M(B)[i][j] * M(u)[j];
        }
    }

    // y = Cx + Du
    for (size_t i = 0; i < M(r); i++) {

        M(y)[i] = 0;

        for (size_t j = 0; j < M(n); j++) {
            M(y)[i] += M(C)[i][j] * M(x)[j];
        }
    }

    for (size_t i = 0; i < M(r); i++) {

        for (size_t j = 0; j < M(m); j++) {
            M(y)[i] += M(D)[i][j] * M(u)[j];
        }
    }

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
        case vr_A:
            ASSERT_NVALUES((size_t)(M(n) * M(n)));
            for (size_t i = 0; i < M(n); i++) {
                for (size_t j = 0; j < M(n); j++) {
                    values[(*index)++] = M(A)[i][j];
                }
            }
            return OK;
        case vr_B:
            ASSERT_NVALUES((size_t)(M(m) * M(n)));
            for (size_t i = 0; i < M(m); i++) {
                for (size_t j = 0; j < M(n); j++) {
                    values[(*index)++] = M(B)[i][j];
                }
            }
            return OK;
        case vr_C:
            ASSERT_NVALUES((size_t)(M(r) * M(n)));
            for (size_t i = 0; i < M(r); i++) {
                for (size_t j = 0; j < M(n); j++) {
                    values[(*index)++] = M(C)[i][j];
                }
            }
            return OK;
        case vr_D:
            ASSERT_NVALUES((size_t)(M(r) * M(m)));
            for (size_t i = 0; i < M(r); i++) {
                for (size_t j = 0; j < M(m); j++) {
                    values[(*index)++] = M(D)[i][j];
                }
            }
            return OK;
        case vr_x0:
            ASSERT_NVALUES((size_t)M(n));
            for (size_t i = 0; i < M(n); i++) {
                values[(*index)++] = M(x0)[i];
            }
            return OK;
        case vr_u:
            ASSERT_NVALUES((size_t)M(m));
            for (size_t i = 0; i < M(m); i++) {
                values[(*index)++] = M(u)[i];
            }
            return OK;
        case vr_y:
            ASSERT_NVALUES((size_t)M(r));
            for (size_t i = 0; i < M(r); i++) {
                values[(*index)++] = M(y)[i];
            }
            return OK;
        case vr_x:
            ASSERT_NVALUES((size_t)M(n));
            for (size_t i = 0; i < M(n); i++) {
                values[(*index)++] = M(x)[i];
            }
            return OK;
        case vr_der_x:
            ASSERT_NVALUES((size_t)M(n));
            for (size_t i = 0; i < M(n); i++) {
                values[(*index)++] = M(der_x)[i];
            }
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
    case vr_A:
        ASSERT_NVALUES((size_t)(M(n) * M(n)));
        for (size_t i = 0; i < M(n); i++) {
            for (size_t j = 0; j < M(n); j++) {
                M(A)[i][j] = values[(*index)++];
            }
        }
        break;
    case vr_B:
        ASSERT_NVALUES((size_t)(M(n) * M(m)));
        for (size_t i = 0; i < M(n); i++) {
            for (size_t j = 0; j < M(m); j++) {
                M(B)[i][j] = values[(*index)++];
            }
        }
        break;
    case vr_C:
        ASSERT_NVALUES((size_t)(M(r) * M(n)));
        for (size_t i = 0; i < M(r); i++) {
            for (size_t j = 0; j < M(n); j++) {
                M(C)[i][j] = values[(*index)++];
            }
        }
        break;
    case vr_D:
        ASSERT_NVALUES((size_t)(M(r) * M(m)));
        for (size_t i = 0; i < M(r); i++) {
            for (size_t j = 0; j < M(m); j++) {
                M(D)[i][j] = values[(*index)++];
            }
        }
        break;
    case vr_x0:
        ASSERT_NVALUES((size_t)M(n));
        for (size_t i = 0; i < M(n); i++) {
            M(x0)[i] = values[(*index)++];
        }
        break;
    case vr_u:
        ASSERT_NVALUES((size_t)M(m));
        for (size_t i = 0; i < M(m); i++) {
            M(u)[i] = values[(*index)++];
        }
        break;
    case vr_x:
        if (comp->state != ContinuousTimeMode && comp->state != EventMode) {
            logError(comp, "Variable \"x\" can only be set in Continuous Time Mode and Event Mode.");
            return Error;
        }
        ASSERT_NVALUES((size_t)M(n));
        for (size_t i = 0; i < M(n); i++) {
            M(x)[i] = values[(*index)++];
        }
        break;
    default:
        logError(comp, "Set Float64 is not allowed for value reference %u.", vr);
        return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status getUInt64(ModelInstance* comp, ValueReference vr, uint64_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_m:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(m);
            return OK;
        case vr_n:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(n);
            return OK;
        case vr_r:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(r);
            return OK;
        default:
            logError(comp, "Get UInt64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setUInt64(ModelInstance* comp, ValueReference vr, const uint64_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    if (comp->state != ConfigurationMode && comp->state != ReconfigurationMode) {
        logError(comp, "Structural variables can only be set in Configuration Mode or Reconfiguration Mode.");
        return Error;
    }

    const uint64_t v = values[(*index)++];

    switch (vr) {
        case vr_m:
            if (v > M_MAX) {
                logError(comp, "Variable m must not be greater than " xstr(M_MAX) ".");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(m) = v;
            break;
        case vr_n:
            if (v > N_MAX) {
                logError(comp, "Variable n must not be greater than " xstr(N_MAX) ".");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(n) = v;
            break;
        case vr_r:
            if (v > R_MAX) {
                logError(comp, "Variable r must not be greater than " xstr(R_MAX) ".");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(r) = v;
            break;
        default:
            logError(comp, "Set UInt64 is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status eventUpdate(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;

    return OK;
}

size_t getNumberOfContinuousStates(ModelInstance* comp) {
    ASSERT_NOT_NULL2(comp);
    return (size_t)M(n);
}

Status getContinuousStates(ModelInstance* comp, double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(x);

    if (nx != M(n)) {
        logError(comp, "Expected nx=%zu but was %zu.", M(n), nx);
        return Error;
    }

    for (size_t i = 0; i < M(n); i++) {
        x[i] = M(x)[i];
    }

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance* comp, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(nominals);

    if (nx != M(n)) {
        logError(comp, "Expected nx=%zu but was %zu.", M(n), nx);
        return Error;
    }

    for (size_t i = 0; i < M(n); i++) {
        nominals[i] = 1.0;
    }

    return OK;
}

Status setContinuousStates(ModelInstance* comp, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(x);

    if (nx != M(n)) {
        logError(comp, "Expected nx=%zu but was %zu.", M(n), nx);
        return Error;
    }

    for (size_t i = 0; i < M(n); i++) {
        M(x)[i] = x[i];
    }

    comp->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance* comp, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(dx);

    if (nx != M(n)) {
        logError(comp, "Expected nx=%zu but was %zu.", M(n), nx);
        return Error;
    }

    calculateValues(comp);

    for (size_t i = 0; i < M(n); i++) {
        dx[i] = M(der_x)[i];
    }

    return OK;
}
