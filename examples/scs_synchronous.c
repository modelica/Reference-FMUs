#include <stdio.h>
#include <math.h>
#include "fmi3Functions.h"
#include "util.h"
#include "config.h"


#define CHECK_STATUS(S) status = S; if (status != fmi3OK) goto TERMINATE;
fmi3Float64 countdownClockIntervals[1] = { 0.0 };
fmi3IntervalQualifier countdownClocksQualifier[1] = { fmi3IntervalNotYetKnown };
fmi3ValueReference vr_countdownClocks[1] = { vr_inClock3 };
fmi3ValueReference outClockVRs[1] = { vr_outClock };
fmi3Clock outClockValues[2];

static fmi3Status recordVariables(fmi3Instance s, fmi3Float64 time) {
    fmi3ValueReference int32VRs[4] = { vr_inClock1Ticks, vr_inClock2Ticks, vr_inClock3Ticks, vr_totalInClockTicks };
    fmi3Int32 values[4] = { 0 };
    fmi3Status status = fmi3GetInt32(s, int32VRs, 4, values, 4);
    printf("%g,%d,%d,%d,%d\n", time, values[0], values[1], values[2], values[3]);
    return status;
}

static void cb_intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment,
                                  fmi3Float64 intermediateUpdateTime,
                                  fmi3Boolean clocksTicked,
                                  fmi3Boolean intermediateVariableSetRequested,
                                  fmi3Boolean intermediateVariableGetAllowed,
                                  fmi3Boolean intermediateStepFinished,
                                  fmi3Boolean canReturnEarly,
                                  fmi3Boolean *earlyReturnRequested,
                                  fmi3Float64 *earlyReturnTime) {

    if (clocksTicked) {
        fmi3Instance *m = ((fmi3Instance *)instanceEnvironment);

        countdownClockIntervals[0] = 0.0;
        countdownClocksQualifier[0] = fmi3IntervalNotYetKnown;
        fmi3GetIntervalDecimal(m, vr_countdownClocks, 1, countdownClockIntervals, countdownClocksQualifier, 1);
    }
}

static void cb_lockPreemption() {
    // nothing to do for synchronous scheduling
}

static void cb_unlockPreemption() {
    // nothing to do for synchronous scheduling
}

int main(int argc, char* argv[]) {

    printf("Running synchronous Scheduled Execution example... ");
    printf("\n");

    fmi3Status status = fmi3OK;

    fmi3Instance m;

    m = fmi3InstantiateScheduledExecution("instance1",           // instanceName
                                          INSTANTIATION_TOKEN,   // instantiationToken
                                          NULL,                  // resourceLocation
                                          fmi3False,             // visible
                                          fmi3False,             // loggingOn
                                          NULL,                  // requiredIntermediateVariables
                                          0,                     // nRequiredIntermediateVariables
                                          &m,                    // instanceEnvironment
                                          cb_logMessage,         // logMessage
                                          cb_intermediateUpdate, // intermediateUpdate
                                          cb_lockPreemption,     // lockPreemption
                                          cb_unlockPreemption);  // unlockPreemption

    if (m == NULL) {
        status = fmi3Error;
        goto TERMINATE;
    }

    CHECK_STATUS(fmi3EnterInitializationMode(m, fmi3False, 0, 0, fmi3False, 0));
    CHECK_STATUS(fmi3ExitInitializationMode(m));

    int time = 0;

    // simulation loop
    while (time < 10) {

        // Model Partition 1 is active every second
        CHECK_STATUS(fmi3ActivateModelPartition(m, vr_inClock1, 0, time));

        // Model Partition 2 is active at 0, 1, 8, and 9
        if (time % 8 == 0 || (time - 1) % 8 == 0) {
            CHECK_STATUS(fmi3ActivateModelPartition(m, vr_inClock2, 0, time));
        }

        if (countdownClocksQualifier[0] == fmi3IntervalChanged) {
            fmi3ActivateModelPartition(m, vr_inClock3, 0, time);
            countdownClocksQualifier[0] = fmi3IntervalUnchanged;
        }

        CHECK_STATUS(fmi3GetClock(m, outClockVRs, 1, outClockValues, 1));

        CHECK_STATUS(recordVariables(m, time));

        time++;
    }

    TERMINATE:

    if (m && status != fmi3Error && status != fmi3Fatal) {
        fmi3Status s = fmi3Terminate(m);
        status = max(status, s);
    }

    if (m && status != fmi3Fatal) {
        // clean up
        fmi3FreeInstance(m);
    }

    printf("done.\n");

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
