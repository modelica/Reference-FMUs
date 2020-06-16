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

void calculateValues(ModelInstance *comp) {

    // y = A * u
    for (int i = 0; i < M(m); i++) {
        M(y)[i] = 0;
        for (int j = 0; j < M(n); j++) {
            M(y)[i] += M(A)[i][j] * M(u)[j];
        }
    }
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {

    calculateValues(comp);

    switch (vr) {
        case vr_u:
            for (int i = 0; i < M(n); i++) {
                value[(*index)++] = M(u)[i];
            }
            return OK;
        case vr_A:
            for (int i = 0; i < M(m); i++) {
            for (int j = 0; j < M(n); j++) {
                value[(*index)++] = M(A)[i][j];
            }}
            return OK;
        case vr_y:
            for (int i = 0; i < M(m); i++) {
                value[(*index)++] = M(y)[i];
            }
            return OK;
        default:
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {
        case vr_u:
            for (int i = 0; i < M(n); i++) {
                M(u)[i] = value[(*index)++];
            }
            calculateValues(comp);
            return OK;
        case vr_A:
            for (int i = 0; i < M(m); i++) {
            for (int j = 0; j < M(n); j++) {
                M(A)[i][j] = value[(*index)++];
            }}
            return OK;
        default:
            return Error;
    }
}

Status getInt32(ModelInstance* comp, ValueReference vr, int *value, size_t *index) {
    calculateValues(comp);
    switch (vr) {
        case vr_m:
            value[(*index)++] = M(m);
            return OK;
        case vr_n:
            value[(*index)++] = M(n);
            return OK;
        default:
            return Error;
    }
}

Status setInt32(ModelInstance* comp, ValueReference vr, const int *value, size_t *index) {

    if (comp->state != ConfigurationMode && comp->state != ReconfigurationMode) {
        return Error;
    }

    int v = value[(*index)++];

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
            return Error;
    }
}

void eventUpdate(ModelInstance *comp) {
    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;
}
