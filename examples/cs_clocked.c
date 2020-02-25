#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <process.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>
#include "fmi3Functions.h"
#include "util.h"
#include "config.h"

// Forward declaration
unsigned __stdcall thr_activateModelPartition(void *args);
static bool anyOutputClockActive(fmi3Instance s);

#define CHECK_STATUS(S) status = S; if (status != fmi3OK) goto out;



//////////////////////////
// Define callback

// Global variables
fmi3IntermediateUpdateInfo updateInfo;

// current simulation time
fmi3Float64 time = 0;

// global arguments for intermediate update operation
ThreadArgs iu_arguments;

// Definition of Inputs
//   => empty in this example
fmi3Int32 inputs[N_INPUTS] = { 100, 1000 };
const fmi3ValueReference vrInputs[N_INPUTS] = { vr_boost_c2, vr_boost_c3 };
fmi3Int32 inputs_c2[1] = { 0 };
fmi3Int32 inputs_c3[1] = { 0 };
const fmi3ValueReference vrInputs_c2[1] = { vr_boost_c2 };
const fmi3ValueReference vrInputs_c3[1] = { vr_boost_c3 };

// Definition of Outputs
fmi3Int32 outputs[N_OUTPUTS] = { 0 };
const fmi3ValueReference vrOutputs[N_OUTPUTS] = { vr_c1Ticks, vr_c2Ticks, vr_c3Ticks, vr_totalInTicks };
fmi3Int32 outputs_c1[2] = { 0 };
fmi3Int32 outputs_c2[1] = { 0 };
fmi3Int32 outputs_c3[1] = { 0 };

const fmi3ValueReference vrOutputs_c1[2] = { vr_c1Ticks, vr_totalInTicks };
const fmi3ValueReference vrOutputs_c2[1] = { vr_c2Ticks };
const fmi3ValueReference vrOutputs_c3[1] = { vr_c3Ticks };

// Definition of Input clocks
fmi3Clock inputClocks[N_INPUT_CLOCKS] = { fmi3ClockInactive };
const fmi3ValueReference vrInputClocks[N_INPUT_CLOCKS] = { vr_c1, vr_c2, vr_c3 };

// Definition of Output clocks
fmi3Clock outputClocks[N_OUTPUT_CLOCKS] = { fmi3ClockInactive };
const fmi3ValueReference vrOutputClocks[N_OUTPUT_CLOCKS] = { vr_c4, vr_c5 };

FILE *outputFile;

fmi3Status recordVariables(fmi3Instance s, fmi3Float64 time) {
	fmi3Status status = fmi3GetClock(s, vrOutputClocks, N_OUTPUT_CLOCKS, outputClocks);
	if (status != fmi3OK) {
		return status;
	}
	status = fmi3GetInt32(s, vrOutputs, N_OUTPUTS, outputs, N_OUTPUTS);
	if (status != fmi3OK) {
		return status;
	}
	fprintf(outputFile, "%g,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", time, inputClocks[InClock_1], inputClocks[InClock_2], inputClocks[InClock_3],
																outputClocks[OutClock_1], outputClocks[OutClock_2],
																outputs_c1[0], outputs_c1[1], outputs_c2[0], outputs_c3[0]);
	
	return status;
}

// Callback
fmi3Status cb_intermediateUpdate(fmi3Instance s, fmi3IntermediateUpdateInfo* intermediateUpdateInfo) {
	HANDLE thr_handle;
	int returnval;

	if (intermediateUpdateInfo->clocksTicked) {

		// some output clock ticked, check for dependend input clocks and, if there are any, fire them

		if (anyOutputClockActive(s)) {
			if (outputClocks[OutClock_1]) {
				// this is clock 4 => fire input clock 3
				iu_arguments.comp = s;
				iu_arguments.clockRef = vrInputClocks[2];
				iu_arguments.retval = &returnval;
				iu_arguments.activationTime = time;
				printf("cb_intermediateUpdate starting thread for clock 3 (vr=%d)\n", iu_arguments.clockRef);

				thr_handle = (HANDLE)_beginthreadex(NULL, 0, thr_activateModelPartition, &iu_arguments, 0, NULL);
				// TODO: set priority
				// TODO: make sure that threads are not executed in parallel. Only one core is allowed to be used.
			}
			if (outputClocks[OutClock_2]) {
				// this is clock 5
				// call some other function, e.g. clock of another FMU...
			}
		}
	}
    return fmi3OK;
}

// Highly sophisticated computation
static fmi3Float64 timeUntilNextEvent() {
	return 1;
}

// Function to determine the stat of all *independent* Input clocks. 
// This depends on the current time (within the simulation) only
// returns true, if any of the input clocks are set and thus a model partition must be activated
static bool anyInputClockActive(fmi3Float64 time) {
    inputClocks[InClock_1] = (int) time % 4 == 0; // active at 0, 4, 8
    inputClocks[InClock_2] = ((int) time % 8 == 0) || (((int) time - 1) % 8 == 0); // active at 0, 1, 8 and 9
	printf("anyInputClockActive: time=%d, inputClocks[InClock_1] = %d inputClocks[InClock_2] = %d\n", 
		(int)time, inputClocks[InClock_1], inputClocks[InClock_2]);
	return inputClocks[InClock_1] || inputClocks[InClock_2];
}

// This function retrieves the state of the output clocks of all partitions of the slave
// outputClocks[] is set accordingly
// returns true if any of the outputClocks is actually set
static bool anyOutputClockActive(fmi3Instance s) {
	int cl;
	for (cl=0; cl< N_OUTPUT_CLOCKS; cl++) 
		outputClocks[cl] = fmi3ClockInactive;

    fmi3GetClock(s, vrOutputClocks, N_OUTPUT_CLOCKS, outputClocks);
    return outputClocks[OutClock_1] || outputClocks[OutClock_2];
}

int main(int argc, char* argv[]) {
    
    fmi3Status status = fmi3OK;
	unsigned int returnval[N_INPUT_CLOCKS]; // return values of all possible threads

    
    const fmi3CallbackFunctions callbacks = {
        .instanceEnvironment = NULL,
        .logMessage          = cb_logMessage,
        .allocateMemory      = cb_allocateMemory,
        .freeMemory          = cb_freeMemory,
        .intermediateUpdate  = cb_intermediateUpdate,
        .lockPreemption      = NULL,
        .unlockPreemption    = NULL
    };
    
    printf("Running Scheduled Execution Co-Simulation example...\n");

    outputFile = fopen("clocks_out.csv", "w");

    if (!outputFile) {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    // write the header of the CSV
    fputs("time,c1,c2,c3,c4,c5,ticks1,ticks2,ticks3,sum \n", outputFile);

    //////////////////////////
    // Initialization sub-phase

    // Set callback functions,
    fmi3EventInfo s_eventInfo;

    //set Co-Simulation mode
    const fmi3CoSimulationConfiguration csConfig = {
        .intermediateVariableGetRequired         = fmi3False,
        .intermediateInternalVariableGetRequired = fmi3False,
        .intermediateVariableSetRequired         = fmi3False,
        .coSimulationMode                        = fmi3ModeScheduledExecutionSimulation
    };

    // Instantiate slave
    const fmi3Instance s = fmi3Instantiate("instance", fmi3CoSimulation, MODEL_GUID, "", &callbacks, fmi3False, fmi3True, &csConfig);

    if (s == NULL) {
        status = fmi3Error;
        goto out;
    }

	
	const fmi3Float64 stopTime = 10; 
    
// Communication constant step size
    fmi3Float64 stepSize; 

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
    CHECK_STATUS(fmi3GetClock(s, vrOutputClocks, N_OUTPUT_CLOCKS, outputClocks));
	stepSize = timeUntilNextEvent();

    bool eventMode = true;
	HANDLE hThread[N_INPUT_CLOCKS];

	int n_handles;
	ThreadArgs arguments[N_INPUT_CLOCKS]; // Need a set of arguments for every Inputclock. otherwise the arguments will be overwritten
	

	while (time < stopTime) {

		if (anyInputClockActive(time)) {
			int i = 0;

			n_handles = 0; // Number of threads that have been fired at this particular point in time

			for (i = 0; i < N_INPUT_CLOCKS; i++)
			{
				if (inputClocks[i]) {
					printf("starting thread for clock %s (vr=%d)\n", i==InClock_1?"InClock_1":"InClock_2", vrInputClocks[i]);
					arguments[i].comp = s;
					arguments[i].clockRef = vrInputClocks[i];
					arguments[i].retval = &returnval[i];
					arguments[i].activationTime = time;

					hThread[n_handles++] = (HANDLE) _beginthreadex(NULL, 0, thr_activateModelPartition, &arguments[i], 0, NULL);
					// TODO: set priority
					// TODO: make sure that threads are not executed in parallel. Only one core is allowed to be used.
				}
			}
			// All needed threads are started, now, we wait, as there's nothing else to do for us 
			// TODO: How long do we have to wait? It should be like the longest time between
			// WaitForMultipleObjects(n_handles, hThread, true, INFINITE);
			
			Sleep(950); // sleep for a little less than step time (???)
		}

		recordVariables(s, time);

		time += stepSize;
	}

    fmi3Status terminateStatus;
out:
    
    if (s && status != fmi3Error && status != fmi3Fatal) {
        terminateStatus = fmi3Terminate(s);
    }
    
    if (s && status != fmi3Fatal && terminateStatus != fmi3Fatal) {
        fmi3FreeInstance(s);
    }
	printf("... finished Scheduled Execution Co-Simulation example.\n");
    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

unsigned __stdcall thr_activateModelPartition(void *args)  {
	ThreadArgs* TA = args;
	fmi3Status retval = fmi3OK;

	// Set Variables of appropriate partition
	switch (TA->clockRef) {
		case vr_c1: {
			printf("activateModelPartition calling fmi3ActivateModelPartition (%d)\n", TA->clockRef);
			// No variables to set for this partition
			retval = fmi3ActivateModelPartition(TA->comp, TA->clockRef, TA->activationTime);
			if (retval != fmi3OK)	break;
			retval = fmi3GetInt32(TA->comp, vrOutputs_c1, 2, outputs_c1, 2);
		}
		break;
		case vr_c2: {
			printf("activateModelPartition calling fmi3ActivateModelPartition (%d)\n", TA->clockRef);
			retval = fmi3SetInt32(TA->comp, vrInputs_c2, 1, inputs_c2, 1);
			if (retval != fmi3OK) break;
			retval = fmi3ActivateModelPartition(TA->comp, TA->clockRef, TA->activationTime);
			if (retval != fmi3OK)	break;
			retval = fmi3GetInt32(TA->comp, vrOutputs_c2, 2, outputs_c2, 1);
		}
		break;
		case vr_c3: {
			printf("activateModelPartition calling fmi3ActivateModelPartition (%d)\n", TA->clockRef);
			retval = fmi3SetInt32(TA->comp, vrInputs_c3, 1, inputs_c3, 1);
			if (retval != fmi3OK)	break;
			retval = fmi3ActivateModelPartition(TA->comp, TA->clockRef, TA->activationTime);
			if (retval != fmi3OK)	break;
			retval = fmi3GetInt32(TA->comp, vrOutputs_c3, 2, outputs_c3, 1);
		}
		break;
		default:
			retval = fmi3Error;
	}
	if (retval != fmi3OK) {
		return ((unsigned int)retval);
	}


// Get Vars of Part

// Output clocks??
	_endthreadex(retval);
	return 0;
}