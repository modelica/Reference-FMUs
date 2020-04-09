#include <stdio.h>
#include <math.h>
#include "fmi3Functions.h"
#include "util.h"
#include "config.h"


#define CHECK_STATUS(S) status = S; if (status != fmi3OK) goto TERMINATE;

static fmi3Status recordVariables(fmi3Instance s, fmi3Float64 time) {
    fmi3ValueReference int32VRs[4] = { vr_inClock1Ticks, vr_inClock2Ticks, vr_inClock3Ticks, vr_totalInClockTicks };
    fmi3Int32 values[4] = { 0 };
    fmi3Status status = fmi3GetInt32(s, int32VRs, 4, values, 4);
    printf("%g,%d,%d,%d,%d\n", time, values[0], values[1], values[2], values[3]);
    return status;
}

static fmi3Status cb_intermediateUpdate(
    fmi3InstanceEnvironment instanceEnvironment,
    fmi3Float64 intermediateUpdateTime,
    fmi3Boolean eventOccurred,
    fmi3Boolean clocksTicked,
    fmi3Boolean intermediateVariableSetAllowed,
    fmi3Boolean intermediateVariableGetAllowed,
    fmi3Boolean intermediateStepFinished,
    fmi3Boolean canReturnEarly) {
    
    fmi3Status status = fmi3OK;
    
    if (clocksTicked) {
        fmi3Instance *m = ((fmi3Instance *)instanceEnvironment);
    
        // ModelPartition 3 depends on inClock1
        fmi3Clock outClock1;
        fmi3ValueReference vr[1] = { vr_outClock1 };
        
        status = fmi3GetClock(m, vr, 1, &outClock1);
        if (status > fmi3OK) return status;
        if (outClock1) {
            // printf("############## Starting task for inClock3\n");
            status = fmi3ActivateModelPartition(m, vr_inClock3, intermediateUpdateTime);
        }
    }
        
    return status;
}

static void cb_lockPreemption() {
    // nothing to do for synchronous scheduling
}

static void cb_unlockPreemption() {
    // nothing to do for synchronous scheduling
}

int main(int argc, char* argv[]) {

    printf("Running synchronous Scheduled Co-Simulation example... ");
    printf("\n");
    
    fmi3Status status = fmi3OK;
    
    fmi3Instance m;

    m = fmi3InstantiateScheduledCoSimulation("instance1",
                                              MODEL_GUID,
                                              NULL,
                                              fmi3False,
                                              fmi3False,
                                              fmi3False,
                                              fmi3False,
                                              fmi3False,
                                              &m,
                                              cb_logMessage,
                                              cb_allocateMemory,
                                              cb_freeMemory,
                                              cb_intermediateUpdate,
                                              cb_lockPreemption,
                                              cb_unlockPreemption);
    
    if (m == NULL) {
        status = fmi3Error;
        goto TERMINATE;
    }

    CHECK_STATUS(fmi3SetupExperiment(m, fmi3False, 0, 0, fmi3False, 0));

    CHECK_STATUS(fmi3EnterInitializationMode(m));
    CHECK_STATUS(fmi3ExitInitializationMode(m));
    
    int time = 0;
    
    fmi3ValueReference outClockVRs[2] = { vr_outClock1, vr_outClock2 };
    fmi3Clock outClockValues[2];

    // simulation loop
    while (time < 10) {
        
        // Model Partition 1 is active every second
        CHECK_STATUS(fmi3ActivateModelPartition(m, vr_inClock1, time));
                
        // Model Partition 2 is active at 0, 1, 8, and 9
        if (time % 8 == 0 || (time - 1) % 8 == 0) {
            CHECK_STATUS(fmi3ActivateModelPartition(m, vr_inClock2, time));
        }
        
        CHECK_STATUS(fmi3GetClock(m, outClockVRs, 2, outClockValues));
                
        CHECK_STATUS(recordVariables(m, time));

        time += 1;
    }

    TERMINATE:
    
    if (m && status != fmi3Error && status != fmi3Fatal) {
        status = max(status, fmi3Terminate(m));
    }
    
    if (m && status != fmi3Fatal) {
        // clean up
        fmi3FreeInstance(m);
    }

    printf("done.\n");

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
