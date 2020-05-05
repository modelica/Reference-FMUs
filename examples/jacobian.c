#include <stdio.h>
#include <assert.h>
#include "fmi3Functions.h"
#include "config.h"
#include "util.h"


int main(int argc, char* argv[]) {

    size_t i, j;
    fmi3Float64 time = 0;
    fmi3Status status = fmi3OK;

    size_t nx = NX;
    fmi3ValueReference vr_x[]  = { vr_x0, vr_x1 };
    fmi3ValueReference vr_dx[] = { vr_der_x0, vr_der_x1 };

    // variables:
    fmi3Instance m;
    fmi3Float64 x[NX];
    fmi3Float64 dk = 1;
    fmi3Float64 J[NX][NX];
    fmi3Float64 c[NX];

    printf("Running Jacobian example... ");

    m = fmi3InstantiateModelExchange("jacobian", INSTANTIATION_TOKEN, NULL, fmi3False, fmi3False, NULL, cb_logMessage);

    if (!m) {
        return EXIT_FAILURE;
    }

    CHECK_STATUS(fmi3EnterInitializationMode(m, fmi3False, 0, 0, fmi3False, 0))
    CHECK_STATUS(fmi3ExitInitializationMode(m))

    CHECK_STATUS(fmi3EnterContinuousTimeMode(m))

    CHECK_STATUS(fmi3GetContinuousStates(m, x, nx))

    // tag::JacobianVariables[]
    // from the XML file:
    //   nx       number of states
    //   vr_x[]   value references of continuous states
    //   vr_xd[]  value references of state derivatives
    //
    // variables:
    //   m        model instance
    //   x[]      continuous states
    //   dk = 1   delta knowns
    //   J[][]    Jacobian
    // end::JacobianVariables[]

    // tag::GetJacobian[]
    //   c[]      column vector

    // set time, states and inputs
    CHECK_STATUS(fmi3SetTime(m, time))
    CHECK_STATUS(fmi3SetContinuousStates(m, x, nx))
    // fmi3Set{VariableType}(m, ...)

    // if required at this step, compute the Jacobian as a dense matrix
    for (i = 0; i < nx; i++) {
        // construct the Jacobian matrix column wise
        CHECK_STATUS(fmi3GetDirectionalDerivative(m, vr_dx, nx, &vr_x[i], 1, &dk, 1, c, nx))
        for (j = 0; j < nx; j++) {
            J[j][i] = c[j];
        }
    }
    // end::GetJacobian[]

    assert(J[0][0] ==  0);
    assert(J[0][1] ==  1);
    assert(J[1][0] == -1);
    assert(J[1][1] == -3);

    // tag::GetJacobianAdjoint[]
    for (i = 0; i < nx; i++) {
        // construct the Jacobian matrix column wise
        CHECK_STATUS(fmi3GetAdjointDerivative(m, &vr_dx[i], 1, vr_x, nx, &dk, 1, &J[i][0], nx))
    }
    // end::GetJacobianAdjoint[]

    assert(J[0][0] ==  0);
    assert(J[0][1] ==  1);
    assert(J[1][0] == -1);
    assert(J[1][1] == -3);

TERMINATE:

    if (status != fmi3Error && status != fmi3Fatal) {
        fmi3Status s = fmi3Terminate(m);
        status = max(status, s);
    }

    if (status != fmi3Fatal) {
        fmi3FreeInstance(m);
    }

    printf("done.\n");

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
