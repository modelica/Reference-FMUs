#include "FMI1.h"
#include "FMI2.h"
#include "FMI3.h"

#include "FMIEuler.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


typedef struct SolverImpl Solver;

struct SolverImpl {
    FMIInstance* S;
    double time;
    size_t nx;
    double* x;
    double* dx;
    size_t nz;
    double* z;
    double* prez;
    FMIStatus(*get_x)(FMIInstance* instance, double x[], size_t nx);
    FMIStatus(*set_x)(FMIInstance* instance, const double x[], size_t nx);
    FMIStatus(*get_dx)(FMIInstance* instance, double dx[], size_t nx);
    FMIStatus(*get_z)(FMIInstance* instance, double z[], size_t nz);
} SolverImpl_;

Solver* FMIEulerCreate(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double startTime) {

    Solver* solver = (Solver*)calloc(1, sizeof(SolverImpl_));

    if (!solver) {
        return NULL;
    }

    solver->S = S;
    solver->time = startTime;

    solver->nx = modelDescription->nContinuousStates;
    solver->x  = (double*)calloc(solver->nx, sizeof(double));
    solver->dx = (double*)calloc(solver->nx, sizeof(double));

    solver->nz   = modelDescription->nEventIndicators;
    solver->z    = (double*)calloc(solver->nx, sizeof(double));
    solver->prez = (double*)calloc(solver->nx, sizeof(double));

    if (S->fmiVersion == FMIVersion1) {
        solver->get_x  = FMI1GetContinuousStates;
        solver->set_x  = FMI1SetContinuousStates;
        solver->get_dx = FMI1GetDerivatives;
        solver->get_z  = FMI1GetEventIndicators;
    } else if (S->fmiVersion == FMIVersion2) {
        solver->get_x  = FMI2GetContinuousStates;
        solver->set_x  = FMI2SetContinuousStates;
        solver->get_dx = FMI2GetDerivatives;
        solver->get_z  = FMI2GetEventIndicators;
    } else if (S->fmiVersion == FMIVersion3) {
        solver->get_x  = FMI3GetContinuousStates;
        solver->set_x  = FMI3SetContinuousStates;
        solver->get_dx = FMI3GetContinuousStateDerivatives;
        solver->get_z  = FMI3GetEventIndicators;
    } else {
        return NULL;
    }

    solver->get_z(solver->S, solver->prez, solver->nz);

    return solver;
}

void FMIEulerFree(Solver* solver) {

    if (!solver) {
        return;
    }

    free(solver->x);
    free(solver->dx);
    free(solver->z);
    free(solver->prez);

    free(solver);
}

FMIStatus FMIEulerStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent) {

    if (!solver) {
        return FMIError;
    }

    FMIStatus status = FMIOK;

    CALL(solver->get_x(solver->S, solver->x, solver->nx));
    CALL(solver->get_dx(solver->S, solver->dx, solver->nx));

    const double dt = nextTime - solver->time;

    for (size_t i = 0; i < solver->nx; i++) {
        solver->x[i] += dt * solver->dx[i];
    }

    CALL(solver->set_x(solver->S, solver->x, solver->nx));
    CALL(solver->get_z(solver->S, solver->z, solver->nz));

    *stateEvent = false;

    for (size_t i = 0; i < solver->nz; i++) {

        if (solver->prez[i] <= 0 && solver->z[i] > 0) {
            *stateEvent = true;  // -\+
        } else if (solver->prez[i] > 0 && solver->z[i] <= 0) {
            *stateEvent = true;  // +/-
        }

        solver->prez[i] = solver->z[i];
    }

    solver->time = nextTime;
    *timeReached = nextTime;

TERMINATE:
    return status;
}

FMIStatus FMIEulerReset(Solver* solver, double time) {
    
    if (!solver) {
        return FMIError;
    }

    return solver->get_z(solver->S, solver->prez, solver->nz);
}
