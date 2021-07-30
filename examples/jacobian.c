#include <assert.h>

#define LOG_FILE "jacobian_log.txt"

#include "util.h"


int main(int argc, char* argv[]) {

    CALL(setUp());

    size_t i, j;
    fmi3Float64 time = 0;

    size_t nx = NX;
    fmi3ValueReference vr_x[]  = { vr_x0, vr_x1 };
    fmi3ValueReference vr_dx[] = { vr_der_x0, vr_der_x1 };

    // variables:
    fmi3Float64 x[NX];
    fmi3Float64 dk = 1;
    fmi3Float64 J[NX][NX];
    fmi3Float64 c[NX];

    CALL(FMI3InstantiateModelExchange(S,
        INSTANTIATION_TOKEN, // instantiationToken
        RESOURCE_PATH,       // resourceLocation
        fmi3False,           // visible
        fmi3False            // loggingOn
    ));

    CALL(FMI3EnterInitializationMode(S, fmi3False, 0, 0, fmi3False, 0));
    CALL(FMI3ExitInitializationMode(S));

    CALL(FMI3EnterContinuousTimeMode(S));

    CALL(FMI3GetContinuousStates(S, x, nx))

    // tag::JacobianVariables[]
    // from the XML file:
    //   nx       number of states
    //   vr_x[]   value references of continuous states
    //   vr_xd[]  value references of state derivatives
    //
    // variables:
    //   s        model instance
    //   x[]      continuous states
    //   dk = 1   delta knowns
    //   J[][]    Jacobian
    // end::JacobianVariables[]

    // tag::GetJacobian[]
    //   c[]      column vector

    // set time, states and inputs
    CALL(FMI3SetTime(S, time));
    CALL(FMI3SetContinuousStates(S, x, nx));
    // fmi3Set{VariableType}(s, ...)

    // if required at this step, compute the Jacobian as a dense matrix
    for (i = 0; i < nx; i++) {
        // construct the Jacobian matrix column wise
        CALL(FMI3GetDirectionalDerivative(S, vr_dx, nx, &vr_x[i], 1, &dk, 1, c, nx))
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
        CALL(FMI3GetAdjointDerivative(S, &vr_dx[i], 1, vr_x, nx, &dk, 1, &J[i][0], nx));
    }
    // end::GetJacobianAdjoint[]

    assert(J[0][0] ==  0);
    assert(J[0][1] ==  1);
    assert(J[1][0] == -1);
    assert(J[1][1] == -3);

TERMINATE:
    return tearDown();
}
