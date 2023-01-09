#include "config.h"
#include "model.h"


void setStartValues(ModelInstance *comp) {

    M(m) = 2;
    M(n) = 2;

    // identity matrix
    for (int i = 0; i < M_MAX; i++) {
    for (int j = 0; j < N_MAX; j++) {
        M(A)[i][j] = i == j ? 1 : 0;
    }}

    for (int i = 0; i < M_MAX; i++) {
        M(u)[i] = i + 1;
    }

    for (int i = 0; i < N_MAX; i++) {
        M(y)[i] = 0;
    }

}

Status calculateValues(ModelInstance *comp) {

    // y = A * u
    for (size_t i = 0; i < M(m); i++) {
        M(y)[i] = 0;
        for (size_t j = 0; j < M(n); j++) {
            M(y)[i] += M(A)[i][j] * M(u)[j];
        }
    }

    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {

    // TODO: check nValues
    UNUSED(nValues);

    calculateValues(comp);

    switch (vr) {
        case vr_time:
            values[(*index)++] = comp->time;
            return OK;
        case vr_u:
            for (size_t i = 0; i < M(n); i++) {
                values[(*index)++] = M(u)[i];
            }
            return OK;
        case vr_A:
            for (size_t i = 0; i < M(m); i++)
            for (size_t j = 0; j < M(n); j++) {
                values[(*index)++] = M(A)[i][j];
            }
            return OK;
        case vr_y:
            for (size_t i = 0; i < M(m); i++) {
                values[(*index)++] = M(y)[i];
            }
            return OK;
        default:
            logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double values[], size_t nValues, size_t* index) {

    // TODO: check nValues
    UNUSED(nValues);

    switch (vr) {
        case vr_u:
            for (size_t i = 0; i < M(n); i++) {
                M(u)[i] = values[(*index)++];
            }
            calculateValues(comp);
            return OK;
        case vr_A:
            for (size_t i = 0; i < M(m); i++)
            for (size_t j = 0; j < M(n); j++) {
                M(A)[i][j] = values[(*index)++];
            }
            return OK;
        default:
            logError(comp, "Set Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status getUInt64(ModelInstance* comp, ValueReference vr, uint64_t values[], size_t nValues, size_t* index) {
    
    ASSERT_NVALUES;
    
    calculateValues(comp);
    
    switch (vr) {
        case vr_m:
            values[(*index)++] = M(m);
            return OK;
        case vr_n:
            values[(*index)++] = M(n);
            return OK;
        default:
            logError(comp, "Get UInt64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setUInt64(ModelInstance* comp, ValueReference vr, const uint64_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES;

    if (comp->state != ConfigurationMode && comp->state != ReconfigurationMode) {
        return Error;
    }

    const uint64_t v = values[(*index)++];

    switch (vr) {
        case vr_m:
            if (v < 1 || v > M_MAX) return Error;
            M(m) = v;
            return OK;
        case vr_n:
            if (v < 1 || v > N_MAX) return Error;
            M(n) = v;
            return OK;
        default:
            logError(comp, "Set UInt64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

void eventUpdate(ModelInstance *comp) {
    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;
}
