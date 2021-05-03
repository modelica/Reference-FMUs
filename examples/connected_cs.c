#include "FMU.h"
#include "util.h"
#include "config.h"

int main(int argc, char* argv[]) {

    fmi3Status status = fmi3OK;

    const fmi3Float64 startTime = 0;
    const fmi3Float64 stopTime = 10;
    const fmi3Float64 h = 1e-2;

    const fmi3ValueReference vr_p1 = vr_fixed_real_parameter;
    const fmi3ValueReference vr_p2 = vr_fixed_real_parameter;
    const fmi3ValueReference vr_u1 = vr_continuous_real_in;
    const fmi3ValueReference vr_u2 = vr_continuous_real_in;
    const fmi3ValueReference vr_y1 = vr_continuous_real_out;
    const fmi3ValueReference vr_y2 = vr_continuous_real_out;

    const fmi3Float64 p1 = 1;
    const fmi3Float64 p2 = 2;

    fmi3Float64 y1 = 0;
    fmi3Float64 y2 = 0;

    fmi3Boolean eventEncountered;
    fmi3Boolean terminateSimulation;
    fmi3Boolean earlyReturn;
    fmi3Float64 lastSuccessfulTime;

    fmi3String GUID1 = INSTANTIATION_TOKEN;
    fmi3String GUID2 = INSTANTIATION_TOKEN;

#if defined(_WIN32)
    const char *sharedLibrary1 = xstr(MODEL_IDENTIFIER) "\\binaries\\x86_64-windows\\" xstr(MODEL_IDENTIFIER) ".dll";
#elif defined(__APPLE__)
    const char *sharedLibrary1 = xstr(MODEL_IDENTIFIER) "/binaries/x86_64-darwin/" xstr(MODEL_IDENTIFIER) ".dylib";
#else
    const char *sharedLibrary1 = xstr(MODEL_IDENTIFIER) "/binaries/x86_64-linux/" xstr(MODEL_IDENTIFIER) ".so";
#endif

    const char *sharedLibrary2 = sharedLibrary1;

    FMU *S1 = loadFMU(sharedLibrary1);

    if (!S1) {
        return EXIT_FAILURE;
    }

    FMU *S2 = loadFMU(sharedLibrary2);

    if (!S2) {
        return EXIT_FAILURE;
    }

    // tag::CoSimulation[]
    ////////////////////////////
    // Initialization sub-phase

    // instantiate both FMUs
    const fmi3Instance s1 = S1->fmi3InstantiateCoSimulation(
        "s1",                // instanceName
        GUID1,               // instantiationToken
        NULL,                // resourceLocation
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        fmi3False,           // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        NULL,                // instanceEnvironment
        cb_logMessage,       // logMessage
        NULL);               // intermediateUpdate

    if (!s1) {
        return EXIT_FAILURE;
    }

    const fmi3Instance s2 = S2->fmi3InstantiateCoSimulation(
        "s2",                // instanceName
        GUID2,               // instantiationToken
        NULL,                // resourceLocation
        fmi3False,           // visible
        fmi3False,           // loggingOn
        fmi3False,           // eventModeUsed
        fmi3False,           // earlyReturnAllowed
        NULL,                // requiredIntermediateVariables
        0,                   // nRequiredIntermediateVariables
        NULL,                // instanceEnvironment
        cb_logMessage,       // logMessage
        NULL);               // intermediateUpdate

    if (!s2) {
        return EXIT_FAILURE;
    }

    // set all variable start values (of "ScalarVariable / <type> / start")
    CHECK_STATUS(S1->fmi3SetFloat64(s1, &vr_p1, 1, &p1, 1))
    CHECK_STATUS(S2->fmi3SetFloat64(s2, &vr_p2, 1, &p2, 1))

    // initialize the FMUs
    CHECK_STATUS(S1->fmi3EnterInitializationMode(s1, fmi3False, 0.0, startTime, fmi3True, stopTime))
    CHECK_STATUS(S2->fmi3EnterInitializationMode(s2, fmi3False, 0.0, startTime, fmi3True, stopTime))

    // set the input values at time = startTime
    CHECK_STATUS(S1->fmi3SetFloat64(s1, &vr_p1, 1, &p1, 1))
    CHECK_STATUS(S2->fmi3SetFloat64(s2, &vr_p2, 1, &p2, 1))

    CHECK_STATUS(S1->fmi3ExitInitializationMode(s1))
    CHECK_STATUS(S2->fmi3ExitInitializationMode(s2))

    ////////////////////////
    // Simulation sub-phase

    for (int step = 0;; step++) {

        // calculate the current time
        const fmi3Float64 time = step * h;

        if (time >= stopTime) {
            break;
        }

        // retrieve outputs
        CHECK_STATUS(S1->fmi3GetFloat64(s1, &vr_y1, 1, &y1, 1))
        CHECK_STATUS(S2->fmi3GetFloat64(s2, &vr_y2, 1, &y2, 1))

        // set inputs
        CHECK_STATUS(S1->fmi3SetFloat64(s1, &vr_u1, 1, &y2, 1))
        CHECK_STATUS(S2->fmi3SetFloat64(s2, &vr_u2, 1, &y1, 1))

        // call instance s1 and check status
        CHECK_STATUS(S1->fmi3DoStep(s1, time, h, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime))

        if (terminateSimulation) {
            printf("Instance s1 requested to terminate simulation.");
            break;
        }

        // call instance s2 and check status as above
        CHECK_STATUS(S2->fmi3DoStep(s2, time, h, fmi3True, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime))

        if (terminateSimulation) {
            printf("Instance s2 requested to terminate simulation.");
            break;
        }
    }

TERMINATE:

    //////////////////////////
    // Shutdown sub-phase

    if (status < fmi3Error) {

        fmi3Status s = S1->fmi3Terminate(s1);

        status = max(status, s);

        if (s < fmi3Fatal) {
            S1->fmi3FreeInstance(s1);
        }
    }

    if (status < fmi3Error) {

        fmi3Status s = S2->fmi3Terminate(s2);

        status = max(status, s);

        if (s < fmi3Fatal) {
            S2->fmi3FreeInstance(s2);
        }
    }
    // end::CoSimulation[]

    freeFMU(S1);
    freeFMU(S2);

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
