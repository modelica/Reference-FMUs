#define OUTPUT_FILE  "scs_synchronous_out.csv"
#define LOG_FILE     "scs_synchronous_log.txt"

#include "util.h"


fmi3Float64 countdownClockIntervals[1] = { 0.0 };
fmi3IntervalQualifier countdownClocksQualifier[1] = { fmi3IntervalNotYetKnown };
fmi3ValueReference vr_countdownClocks[1] = { vr_inClock3 };
fmi3ValueReference outClockVRs[1] = { vr_outClock };
fmi3Clock outClockValues[2];


static void cb_clockUpdate(fmi3InstanceEnvironment instanceEnvironment) {
    countdownClockIntervals[0] = 0.0;
    countdownClocksQualifier[0] = fmi3IntervalNotYetKnown;
    FMI3GetIntervalDecimal(S, vr_countdownClocks, 1, countdownClockIntervals, countdownClocksQualifier, 1);
}


int main(int argc, char* argv[]) {

    CALL(setUp());

    CALL(FMI3InstantiateScheduledExecution(S,
        INSTANTIATION_TOKEN,   // instantiationToken
        NULL,                  // resourceLocation
        fmi3False,             // visible
        fmi3False,             // loggingOn
        NULL,                  // requiredIntermediateVariables
        0,                     // nRequiredIntermediateVariables
        cb_clockUpdate,        // clockUpdate
        NULL,                  // lockPreemption
        NULL                   // unlockPreemption
    ));

    //environment.S = S;
    //environment.s = s;

    CALL(FMI3EnterInitializationMode(S, fmi3False, 0, 0, fmi3False, 0));
    CALL(FMI3ExitInitializationMode(S));

    int time = 0;

    // simulation loop
    while (time < 10) {

        // Model Partition 1 is active every second
        CALL(FMI3ActivateModelPartition(S, vr_inClock1, 0, time));

        // Model Partition 2 is active at 0, 1, 8, and 9
        if (time % 8 == 0 || (time - 1) % 8 == 0) {
            CALL(FMI3ActivateModelPartition(S, vr_inClock2, 0, time));
        }

        if (countdownClocksQualifier[0] == fmi3IntervalChanged) {
            CALL(FMI3ActivateModelPartition(S, vr_inClock3, 0, time));
            countdownClocksQualifier[0] = fmi3IntervalUnchanged;
        }

        CALL(FMI3GetClock(S, outClockVRs, 1, outClockValues, 1));

        CALL(recordVariables(S, outputFile));

        time++;
    }

TERMINATE:
    return tearDown();
}
