#include "config.h"
#include "model_instance.h"


Status setStartValues(ModelInstance *instance) {
    ASSERT_NOT_NULL2(instance);

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

    instance->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {
    ASSERT_NOT_NULL2(instance);

    // der(x) = A * x + B * u
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

    // y = C * x + D * u
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

    instance->isDirtyValues = false;

    return OK;
}

Status getVariableSize(ModelInstance* instance, ValueReference vr, size_t* size) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(size);

    switch (vr) {
    case vr_time:
        *size = 1;
        break;
    case vr_m:
    case vr_n:
    case vr_r:
        *size = 1;
        break;
    case vr_A:
        *size = (size_t)(M(n) * M(n));
        break;
    case vr_B:
        *size = (size_t)(M(m) * M(n));
        break;
    case vr_C:
        *size = (size_t)(M(r) * M(n));
        break;
    case vr_D:
        *size = (size_t)(M(r) * M(m));
        break;
    case vr_x0:
    case vr_x:
    case vr_der_x:
        *size = (size_t)M(n);
        break;
    case vr_u:
        *size = (size_t)M(m);
        break;
    case vr_y:
        *size = (size_t)M(r);
        break;
    default:
        logError(instance, "Unknown value reference: %u.", vr);
        return Error;
    }
    return OK;
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
            logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance *instance, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
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
        if (instance->state != ContinuousTimeMode && instance->state != EventMode) {
            logError(instance, "Variable \"x\" can only be set in Continuous Time Mode and Event Mode.");
            return Error;
        }
        ASSERT_NVALUES((size_t)M(n));
        for (size_t i = 0; i < M(n); i++) {
            M(x)[i] = values[(*index)++];
        }
        break;
    default:
        logError(instance, "Set Float64 is not allowed for value reference %u.", vr);
        return Error;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getUInt64(ModelInstance *instance, ValueReference vr, uint64_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
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
            logError(instance, "Get UInt64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setUInt64(ModelInstance *instance, ValueReference vr, const uint64_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    if (instance->state != ConfigurationMode && instance->state != ReconfigurationMode) {
        logError(instance, "Structural variables can only be set in Configuration Mode or Reconfiguration Mode.");
        return Error;
    }

    uint64_t v;

    switch (vr) {
        case vr_m:
            ASSERT_NVALUES(1);
            v = values[(*index)++];
            if (v > M_MAX) {
                logError(instance, "Variable m must not be greater than " xstr(M_MAX) ".");
                return Error;
            }
            M(m) = v;
            break;
        case vr_n:
            ASSERT_NVALUES(1);
            v = values[(*index)++];
            if (v > N_MAX) {
                logError(instance, "Variable n must not be greater than " xstr(N_MAX) ".");
                return Error;
            }
            M(n) = v;
            break;
        case vr_r:
            ASSERT_NVALUES(1);
            v = values[(*index)++];
            if (v > R_MAX) {
                logError(instance, "Variable r must not be greater than " xstr(R_MAX) ".");
                return Error;
            }
            M(r) = v;
            break;
        default:
            logError(instance, "Set UInt64 is not allowed for value reference %u.", vr);
            return Error;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getNumberOfContinuousStates(ModelInstance* instance, size_t* nContinuousStates) {
    ASSERT_NOT_NULL2(instance);
    *nContinuousStates = (size_t)M(n);
    return OK;
}

Status getContinuousStates(ModelInstance *instance, double x[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(x);

    if (nx != M(n)) {
        logError(instance, "Expected nx=%zu but was %zu.", M(n), nx);
        return Error;
    }

    for (size_t i = 0; i < M(n); i++) {
        x[i] = M(x)[i];
    }

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance *instance, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(nominals);

    if (nx != M(n)) {
        logError(instance, "Expected nx=%zu but was %zu.", M(n), nx);
        return Error;
    }

    for (size_t i = 0; i < M(n); i++) {
        nominals[i] = 1.0;
    }

    return OK;
}

Status setContinuousStates(ModelInstance *instance, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(x);

    if (nx != M(n)) {
        logError(instance, "Expected nx=%zu but was %zu.", M(n), nx);
        return Error;
    }

    for (size_t i = 0; i < M(n); i++) {
        M(x)[i] = x[i];
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *instance, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(dx);

    if (nx != M(n)) {
        logError(instance, "Expected nx=%zu but was %zu.", M(n), nx);
        return Error;
    }

    calculateValues(instance);

    for (size_t i = 0; i < M(n); i++) {
        dx[i] = M(der_x)[i];
    }

    return OK;
}

Status getPartialDerivative(ModelInstance *instance, ValueReference unknown, size_t unknownIndex, ValueReference known, size_t knownIndex, double *partialDerivative) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(partialDerivative);

    const size_t n = (size_t)M(n);
    const size_t m = (size_t)M(m);
    const size_t r = (size_t)M(r);

    calculateValues(instance);

    *partialDerivative = 0.0;

    if (unknown == vr_der_x && known == vr_x) {
        if (unknownIndex < n && knownIndex < n) {
            *partialDerivative = M(A)[unknownIndex][knownIndex];
        }
    } else if (unknown == vr_der_x && known == vr_u) {
        if (unknownIndex < n && knownIndex < m) {
            *partialDerivative = M(B)[unknownIndex][knownIndex];
        }
    } else if (unknown == vr_y && known == vr_x) {
        if (unknownIndex < r && knownIndex < n) {
            *partialDerivative = M(C)[unknownIndex][knownIndex];
        }
    } else if (unknown == vr_y && known == vr_u) {
        if (unknownIndex < r && knownIndex < m) {
            *partialDerivative = M(D)[unknownIndex][knownIndex];
        }
    } else if (unknown == vr_der_x && known == vr_A) {
        if (unknownIndex < n && knownIndex < n * n) {
            const size_t row = knownIndex % n;
            const size_t col = knownIndex / n;
            if (row < n && col < n && unknownIndex == row) {
                *partialDerivative = M(x)[col];
            }
        }
    } else if (unknown == vr_der_x && known == vr_B) {
        if (unknownIndex < n && knownIndex < n * m) {
            const size_t row = knownIndex % n;
            const size_t col = knownIndex / n;
            if (row < n && col < m && unknownIndex == row) {
                *partialDerivative = M(u)[col];
            }
        }
    } else if (unknown == vr_y && known == vr_C) {
        if (unknownIndex < r && knownIndex < r * n) {
            const size_t row = knownIndex % r;
            const size_t col = knownIndex / r;
            if (row < r && col < n && unknownIndex == row) {
                *partialDerivative = M(x)[col];
            }
        }
    } else if (unknown == vr_y && known == vr_D) {
        if (unknownIndex < r && knownIndex < r * m) {
            const size_t row = knownIndex % r;
            const size_t col = knownIndex / r;
            if (row < r && col < m && unknownIndex == row) {
                *partialDerivative = M(u)[col];
            }
        }
    }

    return OK;
}
