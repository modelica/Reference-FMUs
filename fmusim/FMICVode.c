#include <cvode/cvode.h>               /* prototypes for CVODE fcts., consts.  */
#include <nvector/nvector_serial.h>    /* access to serial N_Vector            */
#include <sunmatrix/sunmatrix_dense.h> /* access to dense SUNMatrix            */
#include <sunlinsol/sunlinsol_dense.h> /* access to dense SUNLinearSolver      */


#include "FMICVode.h"
#include "FMI2.h"
#include "FMI3.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


typedef struct SolverImpl Solver;

struct SolverImpl {
    FMIInstance* S;
    const FMUStaticInput* input;
    size_t nx;
    size_t nz;
    SUNContext sunctx;
    N_Vector y;
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

#define RTOL  RCONST(1.0e-4)   /* scalar relative tolerance            */


// Right-hand-side function
static int f(realtype t, N_Vector y, N_Vector ydot, void* user_data) {

    Solver* solver = (Solver*)user_data;

    solver->set_time(solver->S, t);

    solver->set_x(solver->S, NV_DATA_S(y), solver->nx);

    solver->get_dx(solver->S, NV_DATA_S(ydot), solver->nx);

    return 0;
}

// Root function
static int g(realtype t, N_Vector y, realtype* gout, void* user_data) {

    Solver* solver = (Solver*)user_data;

    solver->set_time(solver->S, t);

    FMIApplyInput(solver->S, solver->input, t, false, true, false);

    solver->set_x(solver->S, NV_DATA_S(y), solver->nx);

    solver->get_z(solver->S, gout, solver->nz);

    return 0;
}

//static int Jac(realtype t, N_Vector y, N_Vector fy, SUNMatrix J, void* user_data, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3);

Solver* CVODECreate(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double startTime) {

    Solver* solver = (Solver*)calloc(1, sizeof(SolverImpl_));

    if (!solver) {
        return NULL;
    }

    solver->S = S;
    solver->input = input;
    
    solver->nx = modelDescription->nContinuousStates;
    solver->nz = modelDescription->nEventIndicators;

    if (S->fmiVersion == FMIVersion2) {
        solver->set_time = FMI2SetTime;
        solver->get_x = FMI2GetContinuousStates;
        solver->set_x = FMI2SetContinuousStates;
        solver->get_dx = FMI2GetDerivatives;
        solver->get_z = FMI2GetEventIndicators;
    } else if (S->fmiVersion == FMIVersion3) {
        solver->set_time = FMI3SetTime;
        solver->get_x = FMI3GetContinuousStates;
        solver->set_x = FMI3SetContinuousStates;
        solver->get_dx = FMI3GetContinuousStateDerivatives;
        solver->get_z = FMI3GetEventIndicators;
    } else {
        return NULL;
    }

    /* Create the SUNDIALS context */
    int retval = SUNContext_Create(NULL, &solver->sunctx);

    /* Initial conditions */
    solver->y = N_VNew_Serial(solver->nx, solver->sunctx);

    /* Initialize y */
    solver->get_x(solver->S, NV_DATA_S(solver->y), solver->nx);

    /* Set the vector absolute tolerance */
    solver->abstol = N_VNew_Serial(solver->nx, solver->sunctx);
    for (size_t i = 0; i < solver->nx; i++) {
        NV_DATA_S(solver->abstol)[i] = RTOL;
    }

    /* Call CVodeCreate to create the solver memory and specify the
     * Backward Differentiation Formula */
    solver->cvode_mem = CVodeCreate(CV_BDF, solver->sunctx);

    CVodeSetUserData(solver->cvode_mem, solver);

    /* Call CVodeInit to initialize the integrator memory and specify the
     * user's right hand side function in y'=f(t,y), the initial time T0, and
     * the initial dependent variable vector y. */
    retval = CVodeInit(solver->cvode_mem, f, startTime, solver->y);

    /* Call CVodeSVtolerances to specify the scalar relative tolerance
     * and vector absolute tolerances */
    retval = CVodeSVtolerances(solver->cvode_mem, RTOL, solver->abstol);

    /* Call CVodeRootInit to specify the root function g with 2 components */
    retval = CVodeRootInit(solver->cvode_mem, (int)solver->nz, g);

    /* Create dense SUNMatrix for use in linear solves */
    solver->A = SUNDenseMatrix(solver->nx, solver->nx, solver->sunctx);

    /* Create dense SUNLinearSolver object for use by CVode */
    solver->LS = SUNLinSol_Dense(solver->y, solver->A, solver->sunctx);

    /* Attach the matrix and linear solver */
    retval = CVodeSetLinearSolver(solver->cvode_mem, solver->LS, solver->A);

    /* Set the user-supplied Jacobian routine Jac */
    //retval = CVodeSetJacFn(cvode_mem, Jac);

    return solver;
}

void CVODEFree(Solver* solver) {

    //free(solver->x);
    //free(solver->dx);
    //free(solver->z);
    //free(solver->prez);

    //free(solver);
}

FMIStatus CVODEStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent) {

    if (!solver) {
        return FMIError;
    }

    FMIStatus status = FMIOK;

    solver->get_x(solver->S, NV_DATA_S(solver->y), solver->nx);

    int flag = CVode(solver->cvode_mem, nextTime, solver->y, timeReached, CV_NORMAL);

    *stateEvent = flag == CV_ROOT_RETURN;

    if (flag < 0) {
        status = FMIError;
    }

TERMINATE:
    return status;
}

FMIStatus CVODEReset(Solver* solver, double time) {
    
    if (!solver) {
        return FMIError;
    }

    solver->get_x(solver->S, NV_DATA_S(solver->y), solver->nx);

    CVodeReInit(solver->cvode_mem, time, solver->y);

    return FMIOK;
}
