#include <stdlib.h>

#include "FMIEuler.h"



#define CALL(f) do { status = f; if (status > FMISolverOK) goto TERMINATE; } while (0)


struct FMISolverImpl {

    double time;
    size_t nx;
    double* x;
    double* dx;
    size_t nz;
    double* z;
    double* prez;

    void* S;
    FMISolverSetTime setTime;
    FMISolverApplyInput applyInput;
    FMISolverGetContinuousStates getContinuousStates;
    FMISolverSetContinuousStates setContinuousStates;
    FMISolverGetContinuousStateDerivatives getContinuousStateDerivatives;
    FMISolverGetEventIndicators getEventIndicators;
    FMISolverLogError logError;

} SolverImpl_Euler;

FMISolver* FMIEulerCreate(const FMISolverParameters* solverFunctions) {

    FMISolverStatus status = FMISolverOK;

    FMISolver* solver = calloc(1, sizeof(SolverImpl_Euler));

    if (!solver) {
        // TODO: log error
        return NULL;
    }

    solver->S = solverFunctions->modelInstance;
    solver->time = solverFunctions->startTime;

    solver->nx = solverFunctions->nx;
    solver->nz = solverFunctions->nz;

    solver->setTime                       = solverFunctions->setTime;
    solver->applyInput                    = solverFunctions->applyInput;
    solver->getContinuousStates           = solverFunctions->getContinuousStates;
    solver->setContinuousStates           = solverFunctions->setContinuousStates;
    solver->getContinuousStateDerivatives = solverFunctions->getContinuousStateDerivatives;
    solver->getEventIndicators            = solverFunctions->getEventIndicators;
    solver->logError                      = solverFunctions->logError;

    if (solver->nx > 0) {
        solver->x  = calloc(solver->nx, sizeof(double));
        solver->dx = calloc(solver->nx, sizeof(double));
    }

    if (solver->nz > 0) {
        solver->z    = calloc(solver->nz, sizeof(double));
        solver->prez = calloc(solver->nz, sizeof(double));
    }

TERMINATE:

    if (status != FMISolverOK) {
        FMIEulerFree(solver);
    }

    return solver;
}

void FMIEulerFree(FMISolver* solver) {

    if (!solver) {
        return;
    }

    if (solver->x)    free(solver->x);
    if (solver->dx)   free(solver->dx);
    if (solver->z)    free(solver->z);
    if (solver->prez) free(solver->prez);

    free(solver);
}

FMISolverStatus FMIEulerStep(FMISolver* solver, double nextTime, double* timeReached, bool* stateEvent) {

    FMISolverStatus status = FMISolverOK;

    const double dt = nextTime - solver->time;

    if (solver->nx > 0) {

        CALL(solver->getContinuousStates(solver->S, solver->x, solver->nx));
        CALL(solver->getContinuousStateDerivatives(solver->S, solver->dx, solver->nx));

        for (size_t i = 0; i < solver->nx; i++) {
            solver->x[i] += dt * solver->dx[i];
        }

        CALL(solver->setContinuousStates(solver->S, solver->x, solver->nx));
    }

    *stateEvent = false;

    if (solver->nz > 0) {

        CALL(solver->getEventIndicators(solver->S, solver->z, solver->nz));

        for (size_t i = 0; i < solver->nz; i++) {

            if (solver->prez[i] <= 0 && solver->z[i] > 0) {
                *stateEvent = true;  // -\+
            } else if (solver->prez[i] > 0 && solver->z[i] <= 0) {
                *stateEvent = true;  // +/-
            }

            solver->prez[i] = solver->z[i];
        }
    }

    solver->time = nextTime;
    *timeReached = nextTime;

TERMINATE:
    return status;
}

FMISolverStatus FMIEulerReset(FMISolver* solver, double time) {

    FMISolverStatus status = FMISolverOK;

    if (solver->nz > 0) {
        CALL(solver->getEventIndicators(solver->S, solver->prez, solver->nz));
    }

TERMINATE:
    return status;
}
