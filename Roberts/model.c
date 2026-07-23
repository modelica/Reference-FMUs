#include <math.h>  // for fabs()
#include "config.h"
#include "model.h"

void setStartValues(ModelInstance *comp) {
    M(y1) = 1;
    M(der_y1) = 0;
    M(y2) = 0;
    M(der_y2) = 0;
    M(y3) = 0;
    M(r) = 0;
}

// dy1 / dt = -.04 * y1 + 1.e4 * y2 * y3
// dy2 / dt = .04 * y1 - 1.e4 * y2 * y3 - 3.e7 * y2 * *2
// 0        = y1 + y2 + y3 - 1

//int resrob(sunrealtype tres, N_Vector yy, N_Vector yp, N_Vector rr,
//    void* user_data)
//{
//    sunrealtype* yval, * ypval, * rval;
//
//    yval = N_VGetArrayPointer(yy);
//    ypval = N_VGetArrayPointer(yp);
//    rval = N_VGetArrayPointer(rr);
//
//    rval[0] = SUN_RCONST(-0.04) * yval[0] + SUN_RCONST(1.0e4) * yval[1] * yval[2];
//    rval[1] = -rval[0] - SUN_RCONST(3.0e7) * yval[1] * yval[1] - ypval[1];
//    rval[0] -= ypval[0];
//    rval[2] = yval[0] + yval[1] + yval[2] - ONE;
//
//    return (0);
//}

Status calculateValues(ModelInstance *comp) {
    M(der_y1) = (-0.04) * M(y1) + 1.e4 * M(y2) * M(y3);
    M(der_y2) = (.04) * M(y1) - 1.e4 * M(y2) * M(y3) - 3.e7 * M(y2) * M(y2);
    M(r) = M(y1) + M(y2) + M(y3) - 1.0;
    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    calculateValues(comp);

    switch (vr) {
        case vr_time:
            values[(*index)++] = comp->time;
            return OK;
        case vr_y1:
            values[(*index)++] = M(y1);
            return OK;
        case vr_der_y1:
            values[(*index)++] = M(der_y1);
            return OK;
        case vr_y2:
            values[(*index)++] = M(y2);
            return OK;
        case vr_der_y2:
            values[(*index)++] = M(der_y2);
            return OK;
        case vr_y3:
            values[(*index)++] = M(y3);
            return OK;
        case vr_r:
            values[(*index)++] = M(r);
            return OK;
        case vr_g1:
            values[(*index)++] = M(g1);
            return OK;
        case vr_g2:
            values[(*index)++] = M(g2);
            return OK;
        default:
            logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double value[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        default:
            logError(comp, "Unexpected value reference: %u.", vr);
            return Error;
    }

    return OK;
}

Status eventUpdate(ModelInstance *comp) {

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

    return OK;
}

size_t getNumberOfEventIndicators(ModelInstance* comp) {
    UNUSED(comp);
    return 2;
}

size_t getNumberOfContinuousStates(ModelInstance* comp) {
    UNUSED(comp);
    return 2;
}

Status getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    UNUSED(nx);
    x[0] = M(y1);
    x[1] = M(y2);
    return OK;
}

Status setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    UNUSED(nx);
    M(y1) = x[0];
    M(y2) = x[1];
    return OK;
}

Status getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    UNUSED(nx);
    dx[0] = M(der_y1);
    dx[1] = M(der_y2);
    return OK;
}

Status getEventIndicators(ModelInstance *comp, double z[], size_t nz) {
    UNUSED(nz);
    z[0] = M(g1);
    z[1] = M(g2);
    return OK;
}
