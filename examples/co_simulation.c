#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#define FMI3_FUNCTION_PREFIX s1_
#include "fmi3Functions.h"
#undef FMI3_FUNCTION_PREFIX

#undef fmi3Functions_h
#define FMI3_FUNCTION_PREFIX s2_
#include "fmi3Functions.h"
#undef FMI3_FUNCTION_PREFIX

#include "util.h"


int main(int argc, char* argv[]) {

    fmi3Float64 startTime, stopTime, h, tc;
    fmi3Status status = fmi3OK;

    const char *guid = "{8c4e810f-3da3-4a00-8276-176fa3c9f000}";

    fmi3Instance s1, s2;

    printf("Running CoSimulation example... ");

// tag::CoSimulation[]
////////////////////////////
// Initialization sub-phase

// instantiate both FMUs
s1 = s1_fmi3InstantiateCoSimulation("instance1",      // instanceName
                                    guid,          // instantiationToken
                                    NULL,          // resourceLocation
                                    fmi3False,     // visible
                                    fmi3False,     // loggingOn
                                    fmi3False,     // eventModeRequired
                                    NULL,          // requiredIntermediateVariables
                                    0,             // nRequiredIntermediateVariables
                                    NULL,          // instanceEnvironment
                                    cb_logMessage, // logMessage
                                    NULL);         // intermediateUpdate

s2 = s2_fmi3InstantiateCoSimulation("instance1",      // instanceName
                                    guid,          // instantiationToken
                                    NULL,          // resourceLocation
                                    fmi3False,     // visible
                                    fmi3False,     // loggingOn
                                    fmi3False,     // eventModeRequired
                                    NULL,          // requiredIntermediateVariables
                                    0,             // nRequiredIntermediateVariables
                                    NULL,          // instanceEnvironment
                                    cb_logMessage, // logMessage
                                    NULL);         // intermediateUpdate

if (s1 == NULL || s2 == NULL)
    return EXIT_FAILURE;

// start and stop time
startTime = 0;
stopTime = 10;

// communication step size
h = 0.01;

// set all variable start values (of "ScalarVariable / <type> / start")
// s1_fmi3SetReal/Integer/Boolean/String(s1, ...);
// s2_fmi3SetReal/Integer/Boolean/String(s2, ...);

// initialize the FMUs
s1_fmi3EnterInitializationMode(s1, fmi3False, 0.0, startTime, fmi3True, stopTime);
s2_fmi3EnterInitializationMode(s2, fmi3False, 0.0, startTime, fmi3True, stopTime);

// set the input values at time = startTime
// fmi3SetReal/Integer/Boolean/String(s1, ...);
// fmi3SetReal/Integer/Boolean/String(s2, ...);

s1_fmi3ExitInitializationMode(s1);
s2_fmi3ExitInitializationMode(s2);

////////////////////////
// Simulation sub-phase
tc = startTime; // current time

while ((tc < stopTime) && (status == fmi3OK)) {

    // retrieve outputs
    // fmi3GetReal(s1, ..., 1, &y1);
    // fmi3GetReal(s2, ..., 1, &y2);

    // set inputs
    // fmi3SetReal(s1, ..., 1, &y2);
    // fmi3SetReal(s2, ..., 1, &y1);

    // call instance s1 and check status
    fmi3Boolean terminate, earlyReturn;
    fmi3Float64 lastSuccessfulTime;

    status = s1_fmi3DoStep(s1, tc, h, fmi3True, &terminate, &earlyReturn, &lastSuccessfulTime);

    if (terminate) {
        printf("Instance s1 requested to terminate simulation.");
        break;
    }

    // call instance s2 and check status as above
    status = s2_fmi3DoStep(s2, tc, h, fmi3True, &terminate, &earlyReturn, &lastSuccessfulTime);

    // ...

    // increment current time
    tc += h;
 }

//////////////////////////
// Shutdown sub-phase
if (status != fmi3Error && status != fmi3Fatal) {
    s1_fmi3Terminate(s1);
    s2_fmi3Terminate(s2);
}

if (status != fmi3Fatal) {
    s1_fmi3FreeInstance(s1);
    s2_fmi3FreeInstance(s2);
}
// end::CoSimulation[]

    printf("done.\n");

    return EXIT_SUCCESS;
}
