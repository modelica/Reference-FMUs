#include <memory.h>
#include "config.h"
#include "model_impl.h"

Status setStartValues(ModelInstance *instance) {
    ASSERT_NOT_NULL(instance);

    M(m) = 3;
    M(n) = 3;
    M(r) = 3;

    // identity matrix
    for (int i = 0; i < M_MAX; i++)
        for (int j = 0; j < N_MAX; j++) {
            M(A)
            [i][j] = i == j ? 1 : 0;
            M(B)
            [i][j] = i == j ? 1 : 0;
            M(C)
            [i][j] = i == j ? 1 : 0;
            M(D)
            [i][j] = i == j ? 1 : 0;
        }

    for (int i = 0; i < M_MAX; i++) {
        M(u)
        [i] = i + 1;
    }

    for (int i = 0; i < N_MAX; i++) {
        M(y)
        [i] = 0;
    }

    for (int i = 0; i < N_MAX; i++) {
        M(x)
        [i] = M(x0)[i];
        M(x)
        [i] = 0;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {
    ASSERT_NOT_NULL(instance);

    // der(x) = A * x + B * u
    for (size_t i = 0; i < M(n); i++) {

        M(der_x)
        [i] = 0;

        for (size_t j = 0; j < M(n); j++) {
            M(der_x)
            [i] += M(A)[i][j] * M(x)[j];
        }
    }

    for (size_t i = 0; i < M(n); i++) {
        for (size_t j = 0; j < M(m); j++) {
            M(der_x)
            [i] += M(B)[i][j] * M(u)[j];
        }
    }

    // y = C * x + D * u
    for (size_t i = 0; i < M(r); i++) {

        M(y)
        [i] = 0;

        for (size_t j = 0; j < M(n); j++) {
            M(y)
            [i] += M(C)[i][j] * M(x)[j];
        }
    }

    for (size_t i = 0; i < M(r); i++) {
        for (size_t j = 0; j < M(m); j++) {
            M(y)
            [i] += M(D)[i][j] * M(u)[j];
        }
    }

    instance->isDirtyValues = false;

    return OK;
}

Status configurate(ModelInstance *instance) {
    ASSERT_NOT_NULL(instance);
#ifdef N_CONTINUOUS_STATES
    Status status = getNumberOfContinuousStates(instance, &instance->nx);
    if (status != OK) {
        return status;
    }
#endif
    return OK;
}

Status getVariableSize(ModelInstance *instance, ValueReference vr, size_t *size) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(size);

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
        return Discard;
    }
    return OK;
}

Status getFloat64(ModelInstance *instance, ValueReference vr, double values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);

    calculateValues(instance);

    switch (vr) {
    case vr_time:
        ASSERT_SIZE_T(nValues, 1);
        values[0] = instance->time;
        return OK;
    case vr_A: {
        const size_t size = (size_t)(M(n) * M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(values, M(A), size * sizeof(double));
        return OK;
    }
    case vr_B: {
        const size_t size = (size_t)(M(m) * M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(values, M(B), size * sizeof(double));
        return OK;
    }
    case vr_C: {
        const size_t size = (size_t)(M(r) * M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(values, M(C), size * sizeof(double));
        return OK;
    }
    case vr_D: {
        const size_t size = (size_t)(M(r) * M(m));
        ASSERT_SIZE_T(nValues, size);
        memcpy(values, M(D), size * sizeof(double));
        return OK;
    }
    case vr_x0: {
        const size_t size = (size_t)(M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(values, M(x0), size * sizeof(double));
        return OK;
    }
    case vr_u: {
        const size_t size = (size_t)(M(m));
        ASSERT_SIZE_T(nValues, size);
        memcpy(values, M(u), size * sizeof(double));
        return OK;
    }
    case vr_y: {
        const size_t size = (size_t)(M(r));
        ASSERT_SIZE_T(nValues, size);
        memcpy(values, M(y), size * sizeof(double));
        return OK;
    }
    case vr_x: {
        const size_t size = (size_t)(M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(values, M(x), size * sizeof(double));
        return OK;
    }
    case vr_der_x: {
        const size_t size = (size_t)(M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(values, M(der_x), size * sizeof(double));
        return OK;
    }
    default:
        logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status setFloat64(ModelInstance *instance, ValueReference vr, const double values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);

    switch (vr) {
    case vr_A: {
        const size_t size = (size_t)(M(n) * M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(M(A), values, size * sizeof(double));
        break;
    }
    case vr_B: {
        const size_t size = (size_t)(M(m) * M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(M(B), values, size * sizeof(double));
        break;
    }
    case vr_C: {
        const size_t size = (size_t)(M(r) * M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(M(C), values, size * sizeof(double));
        break;
    }
    case vr_D: {
        const size_t size = (size_t)(M(r) * M(m));
        ASSERT_SIZE_T(nValues, size);
        memcpy(M(D), values, size * sizeof(double));
        break;
    }
    case vr_x0: {
        const size_t size = (size_t)(M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(M(x0), values, size * sizeof(double));
        break;
    }
    case vr_u: {
        const size_t size = (size_t)(M(m));
        ASSERT_SIZE_T(nValues, size);
        memcpy(M(u), values, size * sizeof(double));
        break;
    }
    case vr_x: {
        const size_t size = (size_t)(M(n));
        ASSERT_SIZE_T(nValues, size);
        memcpy(M(x), values, size * sizeof(double));
        break;
    }
    default:
        logError(instance, "Set Float64 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getUInt64(ModelInstance *instance, ValueReference vr, uint64_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_m:
        values[0] = M(m);
        return OK;
    case vr_n:
        values[0] = M(n);
        return OK;
    case vr_r:
        values[0] = M(r);
        return OK;
    default:
        logError(instance, "Get UInt64 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status setUInt64(ModelInstance *instance, ValueReference vr, const uint64_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    if (instance->state != ConfigurationMode && instance->state != ReconfigurationMode) {
        logError(instance, "Structural variables can only be set in Configuration Mode or Reconfiguration Mode.");
        return Discard;
    }

    const uint64_t v = values[0];

    switch (vr) {
    case vr_m:
        if (v > M_MAX) {
            logError(instance, "Variable 'm' must not be greater than " xstr(M_MAX) ".");
            return Discard;
        }
        M(m) = v;
        break;
    case vr_n:
        if (v > N_MAX) {
            logError(instance, "Variable 'n' must not be greater than " xstr(N_MAX) ".");
            return Discard;
        }
        M(n) = v;
        break;
    case vr_r:
        if (v > R_MAX) {
            logError(instance, "Variable 'r' must not be greater than " xstr(R_MAX) ".");
            return Discard;
        }
        M(r) = v;
        break;
    default:
        logError(instance, "Set UInt64 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getNumberOfContinuousStates(ModelInstance *instance, size_t *nContinuousStates) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(nContinuousStates);
    *nContinuousStates = (size_t)M(n);
    return OK;
}

Status getContinuousStates(ModelInstance *instance, double x[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(x);

    if (nx != M(n)) {
        logError(instance, "Expected nx=%zu but was %zu.", M(n), nx);
        return Discard;
    }

    for (size_t i = 0; i < M(n); i++) {
        x[i] = M(x)[i];
    }

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance *instance, double nominals[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(nominals);

    if (nx != M(n)) {
        logError(instance, "Expected nx=%zu but was %zu.", M(n), nx);
        return Discard;
    }

    for (size_t i = 0; i < M(n); i++) {
        nominals[i] = 1.0;
    }

    return OK;
}

Status setContinuousStates(ModelInstance *instance, const double x[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(x);

    if (nx != M(n)) {
        logError(instance, "Expected nx=%zu but was %zu.", M(n), nx);
        return Discard;
    }

    for (size_t i = 0; i < M(n); i++) {
        M(x)
        [i] = x[i];
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *instance, double dx[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(dx);

    if (nx != M(n)) {
        logError(instance, "Expected nx=%zu but was %zu.", M(n), nx);
        return Discard;
    }

    calculateValues(instance);

    for (size_t i = 0; i < M(n); i++) {
        dx[i] = M(der_x)[i];
    }

    return OK;
}

Status getPartialDerivative(ModelInstance *instance, ValueReference unknown, size_t unknownIndex, ValueReference known, size_t knownIndex, double *partialDerivative) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(partialDerivative);

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

Status getNumberOfEventIndicators(ModelInstance *instance, size_t *nEventIndicators) {
    UNUSED(instance);
    UNUSED(nEventIndicators);
    *nEventIndicators = 0;
    return OK;
}

Status getEventIndicators(ModelInstance *instance, double z[], size_t nz) {
    UNUSED(instance);
    UNUSED(z);
    ASSERT_SIZE_T(nz, 0);
    return OK;
}
