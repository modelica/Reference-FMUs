#include "ForwardEuler.h"


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
} SolverImpl_;

Solver* ForwardEulerCreate(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double startTime) {

    Solver* solver = (Solver*)calloc(1, sizeof(SolverImpl_));

    if (!solver) {
        return NULL;
    }

    solver->S = S;
    solver->time = startTime;

    solver->nx = modelDescription->nContinuousStates;
    solver->x = (double*)calloc(solver->nx, sizeof(double));
    solver->dx = (double*)calloc(solver->nx, sizeof(double));

    solver->nz = modelDescription->nEventIndicators;
    solver->z = (double*)calloc(solver->nx, sizeof(double));
    solver->prez = (double*)calloc(solver->nx, sizeof(double));

    FMI2GetEventIndicators(solver->S, solver->prez, solver->nz);

    return solver;
}

void ForwardEulerFree(Solver* solver) {

    free(solver->x);
    free(solver->dx);
    free(solver->z);
    free(solver->prez);

    free(solver);
}

void ForwardEulerStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent) {

    if (!solver) {
        return;
    }

    FMI2GetContinuousStates(solver->S, solver->x, solver->nx);

    FMI2GetDerivatives(solver->S, solver->dx, solver->nx);

    const double dt = nextTime - solver->time;

    for (size_t i = 0; i < solver->nx; i++) {
        solver->x[i] += dt * solver->dx[i];
    }

    FMI2SetContinuousStates(solver->S, solver->x, solver->nx);

    FMI2GetEventIndicators(solver->S, solver->z, solver->nz);

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
}

void ForwardEulerReset(Solver* solver, double time) {

    FMI2GetEventIndicators(solver->S, solver->prez, solver->nz);
}
