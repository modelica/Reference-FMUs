#include "config.h"
#include "model.h"


void setStartValues(ModelInstance *comp) {

    M(m) = 2;

    M(n) = 3;

    M(A)[0][0] = 1; M(A)[0][1] = 1; M(A)[0][2] = 0;
    M(A)[1][0] = 0; M(A)[1][1] = 1; M(A)[1][2] = 1;

    M(u)[0] = 0;
    M(u)[1] = 1;
    M(u)[2] = 2;
    
    M(y)[0] = 0;
    M(y)[1] = 0;
}

void calculateValues(ModelInstance *comp) {
    
    M(y)[0] = M(A)[0][0] * M(u)[0] + M(A)[0][1] * M(u)[1] + M(A)[0][2] * M(u)[2];
    M(y)[1] = M(A)[1][0] * M(u)[0] + M(A)[1][1] * M(u)[1] + M(A)[1][2] * M(u)[2];

}

Status getReal(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    
    //calculateValues(comp);
    
//    size_t i = *index;
//    size_t *ii = &i;
//    *index = 0;
    
    logError(comp, "getReal(vr=%d)", vr);
    
    switch (vr) {
        case vr_m:
            value[(*index)++] = M(m);
            return OK;
        case vr_n:
            value[(*index)++] = M(n);
            return OK;
        case vr_u:
            value[(*index)++] = M(u)[0];
            value[(*index)++] = M(u)[1];
            value[(*index)++] = M(u)[2];
            return OK;
        case vr_A:
            value[(*index)++] = M(A)[0][0];
            value[(*index)++] = M(A)[0][1];
            value[(*index)++] = M(A)[0][2];
            value[(*index)++] = M(A)[1][0];
            value[(*index)++] = M(A)[1][1];
            value[(*index)++] = M(A)[1][2];
            return OK;
        case vr_y:
            value[0] = (*index)++;
            //(*index) += 1; //M(y)[0];
            value[1] = (*index)++; //M(y)[1];
            return OK;
        default:
            return Error;
    }
}

Status setReal(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {
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
