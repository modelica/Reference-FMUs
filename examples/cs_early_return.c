#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "fmi3Functions.h"
#include "callbacks.h"
#include "config.h"


#define CHECK_STATUS(S) status = S; if (status != fmi3OK) goto out;


typedef struct {
    fmi3Instance instance;
    FILE* outputFile;
    fmi3Float64 intermediateUpdateTime;
} InstanceEnvironment;

fmi3Status recordVariables(InstanceEnvironment instanceEnvironment, fmi3Float64 time) {
    fmi3ValueReference outputsVRs[2] = { vr_h, vr_v };
    fmi3Float64 y[2];
    fmi3Status status = fmi3GetFloat64(instanceEnvironment.instance, outputsVRs, 2, y, 2);
    fprintf(instanceEnvironment.outputFile, "%g,%g,%g\n", time, y[0], y[1]);
    return status;
}

//////////////////////////
// Define callback

// Global variables
//fmi3IntermediateUpdateInfo s_intermediateInfo;

// Callback
fmi3Status cb_intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment, fmi3IntermediateUpdateInfo* intermediateUpdateInfo) {
    InstanceEnvironment* env = (InstanceEnvironment*)instanceEnvironment;
    // remember intermediateUpdateTime
    env->intermediateUpdateTime = intermediateUpdateInfo->intermediateUpdateTime;
    // stop here
    return fmi3DoEarlyReturn(env->instance, env->intermediateUpdateTime);
}

int main(int argc, char* argv[]) {
    
    puts("Running BouncingBall test... ");
    
    // Start and stop time
    const fmi3Float64 startTime = 0;
    const fmi3Float64 stopTime = 3;
    // Communication constant step size
    const fmi3Float64 h = 0.01;
    
    InstanceEnvironment instanceEnvironment = {
        .instance               = NULL,
        .outputFile             = NULL,
        .intermediateUpdateTime = startTime
    };
    
    instanceEnvironment.outputFile = fopen("BouncingBall_out.csv", "w");
    
    if (!instanceEnvironment.outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }
    
    // write the header of the CSV
    fputs("time,h,v\n", instanceEnvironment.outputFile);
    
    fmi3CallbackFunctions callbacks = {
        .allocateMemory     = cb_allocateMemory,
        .freeMemory         = cb_freeMemory,
        .logMessage         = cb_logMessage,
        .intermediateUpdate = cb_intermediateUpdate,
        .lockPreemption     = NULL,
        .unlockPreemption   = NULL
    };
    
    //////////////////////////
    // Initialization sub-phase
    
    fmi3EventInfo eventInfo;
    
    // Create pointer to information for identifying the FMU in callbacks
    callbacks.instanceEnvironment = &instanceEnvironment;
    
    //set Co-Simulation mode
    fmi3CoSimulationConfiguration csConfig = {
        .intermediateVariableGetRequired         = fmi3False,
        .intermediateInternalVariableGetRequired = fmi3False,
        .intermediateVariableSetRequired         = fmi3False,
        .coSimulationMode                        = fmi3ModeHybridCoSimulation
    };
    
    // Instantiate slave
    fmi3Instance s = fmi3Instantiate("instance", fmi3CoSimulation, MODEL_GUID, "", &callbacks, fmi3False, fmi3False, &csConfig);
    
    if (s == NULL) {
        puts("Failed to instantiate FMU.");
        return EXIT_FAILURE;
    }
    
    instanceEnvironment.instance = s;
    
    // Set all variable start values (of "ScalarVariable / <type> / start")
    // fmi3SetReal/Integer/Boolean/String(s, ...);
    
    fmi3Status status = fmi3OK;

    // Initialize slave
    CHECK_STATUS(fmi3SetupExperiment(s, fmi3False, 0.0, startTime, fmi3True, stopTime))
    CHECK_STATUS(fmi3EnterInitializationMode(s))
    // Set the input values at time = startTime
    // fmi3SetReal/Integer/Boolean/String(s, ...);
    CHECK_STATUS(fmi3ExitInitializationMode(s))
    
    //////////////////////////
    // Simulation sub-phase
    fmi3Float64 tc = startTime; // Starting master time
    fmi3Float64 step = h;       // Starting non-zero step size
    
    while (tc < stopTime) {
        
        if (step > 0) {
            // Continuous mode (default mode)
            fmi3Boolean earlyReturn = fmi3False;
            
            status = fmi3DoStep(s, tc, step, fmi3False, &earlyReturn);
            
            switch (status) {
                case fmi3OK:
                    if (earlyReturn) {
                        // TODO: pass reasons
                        CHECK_STATUS(fmi3EnterEventMode(s, fmi3False, fmi3False, NULL, 0, fmi3False));
                        step = 0;
                        tc = instanceEnvironment.intermediateUpdateTime;
                    } else {
                        tc += step;
                        step = h;
                    }
                    break;
                case fmi3Discard:
                    // TODO: handle discard
                    break;
                default:
                    CHECK_STATUS(status)
                    break;
            };
        } else {
            // Event mode
            CHECK_STATUS(fmi3NewDiscreteStates(s, &eventInfo))
            if (!eventInfo.newDiscreteStatesNeeded) {
                CHECK_STATUS(fmi3EnterContinuousTimeMode(s))
                step = h - fmod(tc, h);  // finish the step
            };
        };
        
        // Get outputs
        // fmi3GetReal/Integer/Boolean/String(s, ...);
        CHECK_STATUS(recordVariables(instanceEnvironment, tc))
        
        // Set inputs
        // fmi3SetReal/Integer/Boolean/String(s, ...);
    };
    
    //////////////////////////
    // Shutdown sub-phase
    fmi3Status terminateStatus;
out:
    
    if (s && status != fmi3Error && status != fmi3Fatal) {
        terminateStatus = fmi3Terminate(s);
    }
    
    if (s && status != fmi3Fatal && terminateStatus != fmi3Fatal) {
        fmi3FreeInstance(s);
    }
    
    puts("done.");
    
    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
