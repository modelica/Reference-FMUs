#include <cvode/cvode.h>
#include <nvector/nvector_serial.h>
#include <sunmatrix/sunmatrix_dense.h>
#include <sunlinsol/sunlinsol_dense.h>

#include "FMICVode.h"

#include "FMI1.h"
#include "FMI2.h"
#include "FMI3.h"


#define RTOL  RCONST(1.0e-4)

#define CALL_CVODE(f) do { flag = f; if (flag < 0) { status = FMIError; goto TERMINATE; } } while (0)

#define CALL_FMI(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

#define ASSERT_NOT_NULL(f) do { if (!f) { status = FMIError; goto TERMINATE; } } while (0)


typedef struct SolverImpl Solver;

struct SolverImpl {
    FMIInstance* S;
    const FMUStaticInput* input;
    size_t nx;
    size_t nz;
    SUNContext sunctx;
    N_Vector x;
    N_Vector abstol;
    SUNMatrix A;
    SUNLinearSolver LS;
    void* cvode_mem;
    FMIStatus(*set_time)(FMIInstance* instance, double time);
    FMIStatus(*get_x)(FMIInstance* instance, double x[], size_t nx);
    FMIStatus(*set_x)(FMIInstance* instance, const double x[], size_t nx);
    FMIStatus(*get_dx)(FMIInstance* instance, double dx[], size_t nx);
    FMIStatus(*get_z)(FMIInstance* instance, double z[], size_t nz);
} SolverImpl_;


// Right-hand-side function
static int f(realtype t, N_Vector x, N_Vector ydot, void* user_data) {

    Solver* solver = (Solver*)user_data;

    FMIStatus status = FMIOK;

    CALL_FMI(solver->set_time(solver->S, t));

    if (solver->nx == 0) {
        NV_DATA_S(ydot)[0] = 0.0;
    } else {
        CALL_FMI(solver->set_x(solver->S, NV_DATA_S(x), NV_LENGTH_S(x)));
        CALL_FMI(solver->get_dx(solver->S, NV_DATA_S(ydot), NV_LENGTH_S(ydot)));
    }

TERMINATE:
    return status > FMIOK ? CV_ERR_FAILURE : CV_SUCCESS;
}

// Root function
static int g(realtype t, N_Vector x, realtype* gout, void* user_data) {

    Solver* solver = (Solver*)user_data;

    FMIStatus status = FMIOK;

    CALL_FMI(solver->set_time(solver->S, t));

    CALL_FMI(FMIApplyInput(solver->S, solver->input, t, false, true, false));

    if (solver->nx > 0) {
        CALL_FMI(solver->set_x(solver->S, NV_DATA_S(x), NV_LENGTH_S(x)));
    }
    
    CALL_FMI(solver->get_z(solver->S, gout, solver->nz));

TERMINATE:
    return status > FMIOK ? CV_ERR_FAILURE : CV_SUCCESS;
}

static int Jac(realtype t, N_Vector y, N_Vector fy, SUNMatrix J, void* user_data, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3) {
    
    Solver* solver = (Solver*)user_data;

    FMIInstance* S = solver->S;

    const fmi2Real dvKnown = 1;

    realtype** cols = SM_COLS_D(J);

    // construct the Jacobian columnwise
    for (size_t i = 0; i < S->nContinuousStates; i++) {
        const FMIStatus status = FMI2GetDirectionalDerivative(S, S->derivativeValueReferences, S->nContinuousStates, &S->continuousStateValueReferences[i], 1, &dvKnown, cols[i]);
        if (status != FMIOK) {
            return -1;
        }
    }

    return 0;
}

Solver* FMICVodeCreate(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double startTime) {

    int flag = CV_SUCCESS;
    FMIStatus status = FMIOK;

    Solver* solver = (Solver*)calloc(1, sizeof(SolverImpl_));

    ASSERT_NOT_NULL(solver);

    solver->S = S;
    solver->input = input;
    
    solver->nx = modelDescription->nContinuousStates;
    solver->nz = modelDescription->nEventIndicators;

    if (S->fmiVersion == FMIVersion1) {
        solver->set_time = FMI1SetTime;
        solver->get_x    = FMI1GetContinuousStates;
        solver->set_x    = FMI1SetContinuousStates;
        solver->get_dx   = FMI1GetDerivatives;
        solver->get_z    = FMI1GetEventIndicators;
    } else if (S->fmiVersion == FMIVersion2) {
        solver->set_time = FMI2SetTime;
        solver->get_x    = FMI2GetContinuousStates;
        solver->set_x    = FMI2SetContinuousStates;
        solver->get_dx   = FMI2GetDerivatives;
        solver->get_z    = FMI2GetEventIndicators;
    } else if (S->fmiVersion == FMIVersion3) {
        solver->set_time = FMI3SetTime;
        solver->get_x    = FMI3GetContinuousStates;
        solver->set_x    = FMI3SetContinuousStates;
        solver->get_dx   = FMI3GetContinuousStateDerivatives;
        solver->get_z    = FMI3GetEventIndicators;
    } else {
        return NULL;
    }

    CALL_CVODE(SUNContext_Create(NULL, &solver->sunctx));

    // insert a dummy state if nx == 0
    solver->x = N_VNew_Serial(solver->nx > 0 ? solver->nx : 1, solver->sunctx);
    ASSERT_NOT_NULL(solver->x);

    if (solver->nx > 0) {
        CALL_FMI(solver->get_x(solver->S, NV_DATA_S(solver->x), solver->nx));
    } else {
        NV_DATA_S(solver->x)[0] = 1.0;
    }

    solver->abstol = N_VNew_Serial(NV_LENGTH_S(solver->x), solver->sunctx);
    ASSERT_NOT_NULL(solver->abstol);

    for (size_t i = 0; i < NV_LENGTH_S(solver->x); i++) {
        NV_DATA_S(solver->abstol)[i] = RTOL;
    }

    solver->cvode_mem = CVodeCreate(CV_BDF, solver->sunctx);
    ASSERT_NOT_NULL(solver->cvode_mem);

    CALL_CVODE(CVodeSetUserData(solver->cvode_mem, solver));

    CALL_CVODE(CVodeInit(solver->cvode_mem, f, startTime, solver->x));

    CALL_CVODE(CVodeSVtolerances(solver->cvode_mem, RTOL, solver->abstol));

    CALL_CVODE(CVodeRootInit(solver->cvode_mem, (int)solver->nz, g));

    solver->A = SUNDenseMatrix(NV_LENGTH_S(solver->x), NV_LENGTH_S(solver->x), solver->sunctx);
    ASSERT_NOT_NULL(solver->A);

    solver->LS = SUNLinSol_Dense(solver->x, solver->A, solver->sunctx);
    ASSERT_NOT_NULL(solver->LS);

    CALL_CVODE(CVodeSetLinearSolver(solver->cvode_mem, solver->LS, solver->A));

    if (modelDescription->modelExchange->providesDirectionalDerivatives) {
        CALL_CVODE(CVodeSetJacFn(solver->cvode_mem, Jac));
    }

TERMINATE:

    if (status > FMIOK) {

        printf("Failed to create CVode.\n");

        FMICVodeFree(solver);

            return NULL;
    }

    return solver;
}

void FMICVodeFree(Solver* solver) {

    if (solver) {

        if (solver->x)         N_VDestroy(solver->x);
        if (solver->abstol)    N_VDestroy(solver->abstol);
        if (solver->cvode_mem) CVodeFree(&solver->cvode_mem);
        if (solver->LS)        SUNLinSolFree(solver->LS);
        if (solver->A)         SUNMatDestroy(solver->A);
        if (solver->sunctx)    SUNContext_Free(&solver->sunctx);

        free(solver);
    }
}

FMIStatus FMICVodeStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent) {

    if (!solver) {
        return FMIError;
    }

    int flag = CV_SUCCESS;
    FMIStatus status = FMIOK;

    if (solver->nx > 0) {
        CALL_FMI(solver->get_x(solver->S, NV_DATA_S(solver->x), NV_LENGTH_S(solver->x)));
    }

    flag = CVode(solver->cvode_mem, nextTime, solver->x, timeReached, CV_NORMAL);

    *stateEvent = flag == CV_ROOT_RETURN;

    if (solver->nx > 0) {
        CALL_FMI(solver->set_x(solver->S, NV_DATA_S(solver->x), NV_LENGTH_S(solver->x)));
    }

    if (flag < 0) {
        status = FMIError;
    }

TERMINATE:
    return status;
}

FMIStatus FMICVodeReset(Solver* solver, double time) {
    
    if (!solver) {
        return FMIError;
    }

    if (solver->nx > 0) {
        solver->get_x(solver->S, NV_DATA_S(solver->x), NV_LENGTH_S(solver->x));
    }

    CVodeReInit(solver->cvode_mem, time, solver->x);

    return FMIOK;
}
