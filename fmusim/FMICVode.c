#include <cvode/cvode.h>
#include <nvector/nvector_serial.h>
#include <sunmatrix/sunmatrix_dense.h>
#include <sunlinsol/sunlinsol_dense.h>

#include "FMICVode.h"


#define CALL_CVODE(f) do { flag = f; if (flag < 0) { status = FMISolverError; goto TERMINATE; } } while (0)

#define CALL_MODEL(f) do { status = f; if (status > FMISolverOK) goto TERMINATE; } while (0)

#define ASSERT_NOT_NULL(f) do { if (!f) { status = FMISolverError; goto TERMINATE; } } while (0)


struct SolverImpl {

    size_t nx;
    size_t nz;

    SUNContext sunctx;
    N_Vector x;
    realtype reltol;
    N_Vector abstol;
    SUNMatrix A;
    SUNLinearSolver LS;
    void* cvode_mem;

    void* S;
    const void* input;
    FMISolverSetTime setTime;
    FMISolverApplyInput applyInput;
    FMISolverGetContinuousStates getContinuousStates;
    FMISolverSetContinuousStates setContinuousStates;
    FMISolverGetNominalsOfContinuousStates getNominalsOfContinuousStates;
    FMISolverGetContinuousStateDerivatives getContinuousStateDerivatives;
    FMISolverGetEventIndicators getEventIndicators;
    FMISolverLogError logError;

} SolverImpl_;

// Right-hand-side function
static int f(realtype t, N_Vector x, N_Vector ydot, void* user_data) {

    Solver* solver = (Solver*)user_data;

    FMISolverStatus status = FMISolverOK;

    CALL_MODEL(solver->setTime(solver->S, t));

    if (solver->nx == 0) {
        NV_DATA_S(ydot)[0] = 0.0;
    } else {
        CALL_MODEL(solver->setContinuousStates(solver->S, NV_DATA_S(x), NV_LENGTH_S(x)));
        CALL_MODEL(solver->getContinuousStateDerivatives(solver->S, NV_DATA_S(ydot), NV_LENGTH_S(ydot)));
    }

TERMINATE:
    return status > FMISolverOK ? CV_ERR_FAILURE : CV_SUCCESS;
}

// Root function
static int g(realtype t, N_Vector x, realtype* gout, void* user_data) {

    Solver* solver = (Solver*)user_data;

    FMISolverStatus status = FMISolverOK;

    CALL_MODEL(solver->setTime(solver->S, t));

    CALL_MODEL(solver->applyInput(solver->S, solver->input, t, false, true, false));

    if (solver->nx > 0) {
        CALL_MODEL(solver->setContinuousStates(solver->S, NV_DATA_S(x), NV_LENGTH_S(x)));
    }
    
    CALL_MODEL(solver->getEventIndicators(solver->S, gout, solver->nz));

TERMINATE:
    return status > FMISolverOK ? CV_ERR_FAILURE : CV_SUCCESS;
}

Solver* FMICVodeCreate(const FMISolverParameters* solverFunctions) {

    int flag = CV_SUCCESS;
    FMISolverStatus status = FMISolverOK;

    Solver* solver = calloc(1, sizeof(SolverImpl_));

    if (!solver) {
        solverFunctions->logError("Failed to allocate memory for solver.");
        return NULL;
    }

    if (solverFunctions->tolerance <= 0) {
        solver->reltol = 1e-4; // default tolerance
    } else {
        solver->reltol = solverFunctions->tolerance;
    }

    solver->S     = solverFunctions->modelInstance;
    solver->input = solverFunctions->input;
    
    solver->nx = solverFunctions->nx;
    solver->nz = solverFunctions->nz;

    solver->setTime                       = solverFunctions->setTime;
    solver->applyInput                    = solverFunctions->applyInput;
    solver->getContinuousStates           = solverFunctions->getContinuousStates;
    solver->setContinuousStates           = solverFunctions->setContinuousStates;
    solver->getNominalsOfContinuousStates = solverFunctions->getNominalsOfContinuousStates;
    solver->getContinuousStateDerivatives = solverFunctions->getContinuousStateDerivatives;
    solver->getEventIndicators            = solverFunctions->getEventIndicators;
    solver->logError                      = solverFunctions->logError;

    CALL_CVODE(SUNContext_Create(NULL, &solver->sunctx));

    solver->cvode_mem = CVodeCreate(CV_BDF, solver->sunctx);
    ASSERT_NOT_NULL(solver->cvode_mem);

    CALL_CVODE(CVodeSetUserData(solver->cvode_mem, solver));

    // insert a dummy state if nx == 0
    solver->x = N_VNew_Serial(solver->nx > 0 ? solver->nx : 1, solver->sunctx);
    ASSERT_NOT_NULL(solver->x);

    if (solver->nx > 0) {
        CALL_MODEL(solver->getContinuousStates(solver->S, NV_DATA_S(solver->x), solver->nx));
    } else {
        NV_Ith_S(solver->x, 0) = 1;
    }

    solver->abstol = N_VNew_Serial(NV_LENGTH_S(solver->x), solver->sunctx);
    ASSERT_NOT_NULL(solver->abstol);

    if (solver->nx > 0) {
        CALL_MODEL(solver->getNominalsOfContinuousStates(solver->S, NV_DATA_S(solver->abstol), solver->nx));
    } else {
        NV_Ith_S(solver->abstol, 0) = 1;
    }

    for (size_t i = 0; i < NV_LENGTH_S(solver->x); i++) {
        NV_Ith_S(solver->abstol, i) *= solver->reltol;
    }

    CALL_CVODE(CVodeInit(solver->cvode_mem, f, solverFunctions->startTime, solver->x));

    CALL_CVODE(CVodeSVtolerances(solver->cvode_mem, solver->reltol, solver->abstol));

    solver->A = SUNDenseMatrix(NV_LENGTH_S(solver->x), NV_LENGTH_S(solver->x), solver->sunctx);
    ASSERT_NOT_NULL(solver->A);

    solver->LS = SUNLinSol_Dense(solver->x, solver->A, solver->sunctx);
    ASSERT_NOT_NULL(solver->LS);

    CALL_CVODE(CVodeSetLinearSolver(solver->cvode_mem, solver->LS, solver->A));

    CALL_CVODE(CVodeRootInit(solver->cvode_mem, (int)solver->nz, g));

TERMINATE:

    if (status > FMISolverOK) {

        FMILogError("Failed to create CVode.\n");

        FMICVodeFree(solver);

        return NULL;
    }

    return solver;
}

void FMICVodeFree(Solver* solver) {

    if (!solver) return;

    if (solver->x)         N_VDestroy(solver->x);
    if (solver->abstol)    N_VDestroy(solver->abstol);
    if (solver->cvode_mem) CVodeFree(&solver->cvode_mem);
    if (solver->LS)        SUNLinSolFree(solver->LS);
    if (solver->A)         SUNMatDestroy(solver->A);
    if (solver->sunctx)    SUNContext_Free(&solver->sunctx);

    free(solver);
}

FMISolverStatus FMICVodeStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent) {

    if (!solver) {
        return FMISolverError;
    }

    int flag = CV_SUCCESS;
    FMISolverStatus status = FMISolverOK;

    if (solver->nx > 0) {
        CALL_MODEL(solver->getContinuousStates(solver->S, NV_DATA_S(solver->x), NV_LENGTH_S(solver->x)));
    }

    flag = CVode(solver->cvode_mem, nextTime, solver->x, timeReached, CV_NORMAL);

    *stateEvent = flag == CV_ROOT_RETURN;

    if (solver->nx > 0) {
        CALL_MODEL(solver->setContinuousStates(solver->S, NV_DATA_S(solver->x), NV_LENGTH_S(solver->x)));
    }

    if (flag < 0) {
        status = FMISolverError;
    }

TERMINATE:
    return status;
}

FMISolverStatus FMICVodeReset(Solver* solver, double time) {

    FMISolverStatus status = FMISolverOK;

    int flag = CV_SUCCESS;

    if (!solver) {
        return FMISolverError;
    }

    if (solver->nx > 0) {

        CALL_MODEL(solver->getContinuousStates(solver->S, NV_DATA_S(solver->x), NV_LENGTH_S(solver->x)));

        CALL_MODEL(solver->getNominalsOfContinuousStates(solver->S, NV_DATA_S(solver->abstol), solver->nx));

        for (size_t i = 0; i < NV_LENGTH_S(solver->x); i++) {
            NV_Ith_S(solver->abstol, i) *= solver->reltol;
        }
    }

    CALL_CVODE(CVodeReInit(solver->cvode_mem, time, solver->x));

TERMINATE:
    return status;
}
