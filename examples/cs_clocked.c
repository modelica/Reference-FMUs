#include <stdio.h>
#include <stdbool.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>
#include "fmi3Functions.h"
#include "util.h"
#include "config.h"


#define CHECK_STATUS(S) status = S; if (status != fmi3OK) goto out;

#define N_INPUT_CLOCKS  2
#define N_OUTPUT_CLOCKS 2
#define N_INPUTS  2
#define N_OUTPUTS 1

//////////////////////////
// Define callback

// Global variables
fmi3IntermediateUpdateInfo updateInfo;
const fmi3ValueReference vr_clocks[N_INPUT_CLOCKS] = { vr_c1, vr_c2 };
fmi3Clock inputClocks[N_INPUT_CLOCKS] = { fmi3ClockInactive };
int clockTick = 0;
fmi3Float64 time = 0;
//const fmi3ValueReference vrInputs[N_INPUTS] = { vr_reference, vr_position };
//fmi3UInt16 inputs[N_INPUTS] = { 0 };
const fmi3ValueReference vrOutputs[N_OUTPUTS] = { vr_totalTicks };
fmi3Int32 outputs[N_OUTPUTS] = { 0 };
const fmi3ValueReference outputClockVRs[N_OUTPUT_CLOCKS] = { vr_c3, vr_c4 };
fmi3Clock outputClocks[N_OUTPUT_CLOCKS] = { fmi3ClockInactive };

FILE *outputFile;

fmi3Status recordVariables(fmi3Instance s) {
	fmi3Status status = fmi3GetClock(s, outputClockVRs, N_OUTPUT_CLOCKS, outputClocks);
	fprintf(outputFile, "%g,%d,%d,%d,%d\n", time, inputClocks[0], inputClocks[1], outputClocks[0], outputClocks[1]);
	return status;
}

// Callback
fmi3Status cb_intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment, fmi3IntermediateUpdateInfo* intermediateUpdateInfo) {
    // save intermediateUpdateInfo for later
    updateInfo = *intermediateUpdateInfo;
    return fmi3OK;
}

//static const fmi3UInt16* calculateInputs() {
//    inputs[0] = UINT16_MAX * sin(4 * M_PI * (time + 10e-3)); // 2 Hz sine with 10 ms phase shift
//    inputs[1] = UINT16_MAX * sin(4 * M_PI * time);           // 2 Hz sine
//    return inputs;
//}

//static const fmi3Boolean* calculateClocks() {
//    inferredClocks[0] = clockTick % 2 == 0; // tick every 2nd step
//    inferredClocks[1] = clockTick % 5 == 0; // tick evary 5th step
//    return inferredClocks;
//}

static bool anyInputClockActive() {
    inputClocks[0] = clockTick % 4 == 0; // active at 4
    inputClocks[1] = clockTick % 8 == 0 || (clockTick - 1) % 8 == 0; // active at 8 and 9
    return inputClocks[0] || inputClocks[1];
}

static fmi3Float64 timeUntilNextEvent() {
    return 1;
}

//static bool anyTriggeredClockActive(fmi3Instance s) {
//	const fmi3ValueReference triggeredClockVRs[N_TRIGGERED_CLOCKS] = { vr_c1, vr_c2 };
//	fmi3Clock triggeredClocks[N_TRIGGERED_CLOCKS] = { fmi3ClockInactive };
//	fmi3GetClock(s, triggeredClockVRs, N_TRIGGERED_CLOCKS, triggeredClocks);
//	return triggeredClocks[0] || triggeredClocks[1];
//}

int main(int argc, char* argv[]) {
    
    fmi3Status status = fmi3OK;
    
    const fmi3CallbackFunctions callbacks = {
        .instanceEnvironment = NULL,
        .logMessage          = cb_logMessage,
        .allocateMemory      = cb_allocateMemory,
        .freeMemory          = cb_freeMemory,
        .intermediateUpdate  = cb_intermediateUpdate,
        .lockPreemption      = NULL,
        .unlockPreemption    = NULL
    };
    
    printf("Running Clocked Co-Simulation example... ");

	outputFile = fopen("clocks_out.csv", "w");

	if (!outputFile) {
		puts("Failed to open output file.");
		return EXIT_FAILURE;
	}

	// write the header of the CSV
	fputs("time,c1,c2,c3,c4\n", outputFile);

    //////////////////////////
    // Initialization sub-phase

    // Set callback functions,
    fmi3EventInfo s_eventInfo;

    //set Co-Simulation mode
    const fmi3CoSimulationConfiguration csConfig = {
        .intermediateVariableGetRequired         = fmi3False,
        .intermediateInternalVariableGetRequired = fmi3False,
        .intermediateVariableSetRequired         = fmi3False,
        .coSimulationMode                        = fmi3ModeHybridCoSimulation
    };

    // Instantiate slave
    const fmi3Instance s = fmi3Instantiate("instance", fmi3CoSimulation, MODEL_GUID, "", &callbacks, fmi3False, fmi3True, &csConfig);

    if (s == NULL) {
        status = fmi3Error;
        goto out;
    }

    // Start and stop time
	const fmi3Float64 stopTime = 10; // 1000e-3; // 1000 ms
    // Communication constant step size
	fmi3Float64 stepSize; // 500e-6; //  500 us

    // set all variable start values

    // Initialize slave
    CHECK_STATUS(fmi3SetupExperiment(s, fmi3False, 0.0, time, fmi3True, stopTime));
    CHECK_STATUS(fmi3EnterInitializationMode(s));
    // Set the input values at time = startTime
//    CHECK_STATUS(fmi3SetUInt16(s, vrInputs, N_INPUTS, calculateInputs(), N_INPUTS))
//    CHECK_STATUS(fmi3ExitInitializationMode(s))

    //////////////////////////
    // Simulation sub-phase
    //fmi3Float64 step = stepSize; // Starting non-zero step size
    
    // update clocks
    anyInputClockActive();
    CHECK_STATUS(fmi3GetClock(s, outputClockVRs, N_OUTPUT_CLOCKS, outputClocks));
    
    bool eventMode = true;
    
    while (time < stopTime) {
                        
        stepSize = timeUntilNextEvent();
        
        if (true || anyInputClockActive()) {
            
            /* set possible active inferred clocks to true or to false*/
            
            if(!eventMode) {
                CHECK_STATUS(fmi3EnterEventMode(s, fmi3False, fmi3False, NULL, 0, fmi3False));
                eventMode = true;
            };
            
            // set input clocks
            CHECK_STATUS(fmi3SetClock(s, vr_clocks, N_INPUT_CLOCKS, inputClocks, NULL));
            
            // fmi3SetInterval(s, ...); /* Only needed if interval changes */
        };

        if (eventMode) {

            // event update
			do {
				CHECK_STATUS(fmi3NewDiscreteStates(s, &s_eventInfo))
			} while (s_eventInfo.newDiscreteStatesNeeded);

            CHECK_STATUS(fmi3EnterStepMode(s));

			eventMode = false;
        }
        
        recordVariables(s);
		
        // Continuous mode (default mode)
        //fmi3Float64 tend = time + step;
        //fmi3Float64 t = tend * 2;
        fmi3Boolean earlyReturn = fmi3False;
        
        CHECK_STATUS(fmi3DoStep(s, time, stepSize, fmi3False, &earlyReturn));
                
//        if (earlyReturn) {
//            //t = updateInfo.intermediateUpdateTime;
//            /* rollback FMUs to earliest event time */
////            CHECK_STATUS(fmi3EnterEventMode(s, fmi3False, fmi3False, NULL, 0, fmi3False));
////            eventMode = true;
////                time = updateInfo.intermediateUpdateTime;
//        } else {
////                time += stepSize;
//        }
                    
        time += 1;

        CHECK_STATUS(fmi3GetClock(s, outputClockVRs, N_OUTPUT_CLOCKS, outputClocks));

        if (updateInfo.clocksTicked) {
			// fmi3GetInterval(s, /*Intervals*/, ...);
        };
        
		clockTick++;
    };

    fmi3Status terminateStatus;
out:
    
    if (s && status != fmi3Error && status != fmi3Fatal) {
        terminateStatus = fmi3Terminate(s);
    }
    
    if (s && status != fmi3Fatal && terminateStatus != fmi3Fatal) {
        fmi3FreeInstance(s);
    }
    
    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
