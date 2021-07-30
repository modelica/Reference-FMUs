#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <strsafe.h>
#include <process.h>
#else
#error This example can only be compiled on Windows
#endif

#include "util.h"


/* *****************  Forward declarations ***************** */
fmi3Status initializeOutputFiles();

unsigned __stdcall thr_activateModelPartition(void *args);

static bool setAndCheckInputClocks(fmi3Float64 time);

static bool checkOutputClocks(FMIInstance *S);

static bool checkCountdownClocks(FMIInstance *S);

void cb_intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment,
    fmi3Float64 intermediateUpdateTime,
    fmi3Boolean clocksTicked,
    fmi3Boolean intermediateVariableSetRequested,
    fmi3Boolean intermediateVariableGetAllowed,
    fmi3Boolean intermediateStepFinished,
    fmi3Boolean canReturnEarly,
    fmi3Boolean *earlyReturnRequested,
    fmi3Float64 *earlyReturnTime);

void cb_lockPreemption();

void cb_unlockPreemption();

void logEvent(const char* message, ...) {

    va_list args;
    va_start(args, message);

    vprintf(message, args);
    printf("\n");

    va_end(args);
}

/* ********************************************************* */

/* *****************  Misc defines ***************** */

//#define START_TIME 0
//#define STOP_TIME 10
// defines for the indexes of the clocks into the inputClocks[] array

#define IDX_IN_CLOCK1 0
#define IDX_IN_CLOCK2 1
#define IDX_IN_CLOCK3 2

#define ID_COUNTDOWN_CLOCK IDX_IN_CLOCK3

// defines for the indexes of the clocks into the outputClocks[] array
#define IDX_OUT_CLOCK 0

/* ********************************************************* */

/* *****************  Global variables ***************** */
// Global lock for protecting the access to the model's memory area
HGLOBAL globalLockVar;

// All threads for the model partitions are supposed to run on the same processor
// stores the to-be-used processor for the thread creation.
DWORD_PTR processorMask;

// current simulation time
fmi3Float64 time = 0;

// Simulation stop time
//const fmi3Float64 stopTime = STOP_TIME;

// Communication step size
fmi3Float64 stepSize = FIXED_SOLVER_STEP;  // => 1

// global arguments for intermediate update operation
typedef struct {
    FMIInstance *S;
    ValueReference clockRef;
    unsigned int* retval;
    double activationTime;
    double delay;
} ThreadArgs;

ThreadArgs iu_arguments;

/* ********************************************************* */

/* *****************  Inputs ***************** */
fmi3Int32 inputs_c2[1] = { 0 };
const fmi3ValueReference vrInputs_c2[1] = { vr_input2 };

/* ********************************************************* */

/* *****************  Outputs ***************** */
// separate the output variables after their clock (== modelpartition)
fmi3Int32 outputs_c1[3] = { 0 };
fmi3Int32 outputs_c2[3] = { 0 };
fmi3Int32 outputs_c3[3] = { 0 };
const fmi3ValueReference vrOutputs_c1[2] = { vr_inClock1Ticks, vr_totalInClockTicks };
const fmi3ValueReference vrOutputs_c2[3] = { vr_inClock2Ticks , vr_totalInClockTicks, vr_result2 };
const fmi3ValueReference vrOutputs_c3[3] = { vr_inClock3Ticks, vr_totalInClockTicks, vr_output3 };

/* ********************************************************* */

/* *****************  Input clocks ***************** */
fmi3Clock inputClocks[N_INPUT_CLOCKS] = { fmi3ClockInactive };
const fmi3ValueReference vrInputClocks[N_INPUT_CLOCKS] = { vr_inClock1, vr_inClock2, vr_inClock3 };
const fmi3ValueReference vrCountdownClocks[1] = { vr_inClock3 };
fmi3Float64 countdownClockIntervals[1] = { 0.0 };
fmi3IntervalQualifier countdownClocksQualifier[1] = { fmi3IntervalNotYetKnown };

/*
 * For FMI3.0 lower priority value  means higher priority
 * In windows lower value means lower prio
 * In windows we only have 5 prio levels [-2 .. +2]
 *
 * Setting the values based on the clock priorities from the ModelDescription.xml
 */
fmi3Int32 inputClockPrio[N_INPUT_CLOCKS] = { 2, 1, -1 };
/* ********************************************************* */

/* *****************  Output clocks ***************** */
fmi3Clock outputClocks[1] = { fmi3ClockInactive };
const fmi3ValueReference vrOutputClocks[1] = { vr_outClock };
/* ********************************************************* */

/* *****************  Misc ***************** */
// Results are printed into these files
FILE *outputFiles[N_INPUT_CLOCKS];


/* ********************************************************* */


int main(int argc, char* argv[]) {

    //fmi3Status status = fmi3OK;
    //fmi3Status terminateStatus = fmi3OK;
    unsigned int returnval[N_INPUT_CLOCKS]; // return values of all possible threads

    //InstanceEnvironment env;

    S = FMICreateInstance("instance1", PLATFORM_BINARY, logMessage, logFunctionCall);

    if (!S) {
        status = fmi3Error;
        goto TERMINATE;
    }

    // Instantiate the FMU
    CALL(FMI3InstantiateScheduledExecution(S,
        INSTANTIATION_TOKEN, // instantiationToken
        RESOURCE_PATH,       // resourcePath
        fmi3True,              // visible,
        fmi3True,              // loggingOn,
        NULL,                  // requiredIntermediateVariables,
        0,                     // nRequiredIntermediateVariables,
        cb_intermediateUpdate, // intermediateUpdate,
        cb_lockPreemption,     // lockPreemption,
        cb_unlockPreemption    // unlockPreemption
    ));

    //env.s = s;

    // Initialize logging
    fmi3String logCategory = "logEvents";

    CALL(FMI3SetDebugLogging(S, fmi3True, 1, &logCategory))

    logEvent("Running Scheduled Execution example...");

    // Get a global lock
    globalLockVar = GlobalAlloc(GMEM_FIXED, sizeof(globalLockVar));
    if (globalLockVar == NULL) {
        status = fmi3Error;
        logEvent("Allocating the global lock failed: %ld", GetLastError());
        goto TERMINATE;
    }

    CALL(initializeOutputFiles());

    // Initialize the instance
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, stopTime))
    CALL(FMI3ExitInitializationMode(S))

    // update clocks
    CALL(FMI3GetClock(S, vrOutputClocks, 1, outputClocks, 1));

    /*
     * Thread related stuff below
     */
    // Handles to access threads after they have been started. One for each input clock
    HANDLE thrHandle[N_INPUT_CLOCKS];

    // Need a set of arguments for every Inputclock. otherwise the arguments will be overwritten
    ThreadArgs thrArguments[2];
    int curThrHandle;

    DWORD_PTR processorNumber = GetCurrentProcessorNumber();
    processorMask = (((DWORD_PTR)1 )<< processorNumber);
    logEvent("GetCurrentProcessorNumber() => %lx (Mask = %lx)", processorNumber, processorMask);

    while (time < stopTime) {

        // According to the current time, the non-dependent input clocks are set
        // the call below returns true, if at least one of the clocks is ticking
        if (setAndCheckInputClocks(time)) {
            logEvent("==========> time = %g", time);

            curThrHandle = 0; // Number of threads that have been fired at this particular point in time
            for (int i = 0; i < 2; i++)
            {
                if (inputClocks[i] == fmi3ClockActive) {
                    logEvent("Starting thread for clock %s (vr=%d)", i == IDX_IN_CLOCK1 ? "InClock_1" : "InClock_2", vrInputClocks[i]);
                    thrArguments[i].S = S;
                    //thrArguments[i].s = S->component;
                    thrArguments[i].clockRef = vrInputClocks[i];
                    thrArguments[i].retval = &returnval[i];
                    thrArguments[i].activationTime = time;
                    thrHandle[curThrHandle] = (HANDLE)_beginthreadex(NULL, 0, thr_activateModelPartition, &thrArguments[i], 0, NULL);
                    if (thrHandle[curThrHandle] != NULL) {
                        SetThreadAffinityMask(thrHandle[curThrHandle], processorMask);
                        SetThreadPriority(thrHandle[curThrHandle], inputClockPrio[i]);
                    }
                    else {
                        logEvent("Could not create thread in main loop");
                        status = fmi3Fatal;
                        goto TERMINATE;
                    }
                    curThrHandle++;
                    inputClocks[i] = fmi3ClockInactive;
                }
            }
            // All needed threads are started, now let the time move forward
        }
#ifdef WINDOWS
        Sleep(950); // sleep for a little less than step time
#endif
        time += stepSize;
    }


TERMINATE:
    return tearDown();

    //if (status < fmi3Fatal) {
    //    terminateStatus = FMI3Terminate(S);
    //}

    //GlobalFree(globalLockVar);

    //for (int part = 0; part < N_INPUT_CLOCKS; part++) {
    //    fclose(outputFile[part]);
    //}

    //logEvent("... finished Scheduled Execution example.");
    //if (S && status < fmi3Fatal && terminateStatus < fmi3Fatal) {
    //    FMI3FreeInstance(S); // After this point, logEvent is no longer possible
    //}

    //return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*
* initializeOutputFiles()
*/
fmi3Status initializeOutputFiles() {
    //char outfile[32];
    //for (int part = 0; part < N_INPUT_CLOCKS; part++)
    //{
    //    sprintf(outfile, "Scheduled_part_%d_out.csv", part + 1);
    //    outputFile[part] = fopen(outfile, "w");
    //    if (!outputFile[part]) {
    //        puts("Failed to open output file.");
    //        return fmi3Error;
    //    }
    //}
    //// write the headers of the CSVs  (we have knowledge about how many partitions we actually have)
    //fputs("time,InClock_1_Ticks,total_InClock_Ticks\n", outputFile[0]);
    //fputs("time,InClock_2_Ticks,total_InClock_Ticks,result\n", outputFile[1]);
    //fputs("time,InClock_3_Ticks,total_InClock_Ticks,output_3\n", outputFile[2]);
    //return fmi3OK;
}

/*
 * recordVariables2(s, time, modelPart)
 * writes the current values of the variables associated to modelPart into the outputFile
 * The status of the all clocks are always printed
 * returns fmi3OK on success
 * returns fmi3Error on failure
 */
fmi3Status recordVariables2(fmi3Instance s, fmi3Float64 time, int modelPart) {

    //if (modelPart == vr_inClock1) {
    //    fprintf(outputFile[0], "%g,%3d,%3d\n", time, outputs_c1[0], outputs_c1[1]);
    //    logEvent("%g,%3d,%3d", time, outputs_c1[0], outputs_c1[1]);
    //}
    //else if (modelPart == vr_inClock2) {
    //    fprintf(outputFile[1], "%g,%3d,%3d,%4d\n", time, outputs_c2[0], outputs_c2[1], outputs_c2[2]);
    //    logEvent("%g,%3d,%3d,%4d", time, outputs_c2[0], outputs_c2[1], outputs_c2[2]);
    //}
    //else if (modelPart == vr_inClock3) {
    //    fprintf(outputFile[2], "%g,%3d,%3d,%4d\n", time, outputs_c3[0], outputs_c3[1], outputs_c3[2]);
    //    logEvent("%g,%3d,%3d,%4d", time, outputs_c3[0], outputs_c3[1], outputs_c3[2]);
    //}
    //return fmi3OK;
}

/*
 * cb_intermediateUpdate()
 *
 * callback function, initiated when a modelpartition calculates the ticking of an output clock
 * This function checks, if any ticking output clock is connected to a dependent input clock.
 * If so, the thread for the respective model partition is started on the designated processor and with the designated priority.
 *
 * returns always fmi3OK (practically a void funtion)
 */
void cb_intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment,
    fmi3Float64 intermediateUpdateTime,
    fmi3Boolean clocksTicked,
    fmi3Boolean intermediateVariableSetRequested,
    fmi3Boolean intermediateVariableGetAllowed,
    fmi3Boolean intermediateStepFinished,
    fmi3Boolean canReturnEarly,
    fmi3Boolean *earlyReturnRequested,
    fmi3Float64 *earlyReturnTime) {

    //InstanceEnvironment *env = (InstanceEnvironment *)instanceEnvironment;

    //FMU *S = env->S;
    //fmi3Instance s = env->s;

    HANDLE thr_handle;
    int returnval;
    // Local copy for this thread of the states of the output clocks
    fmi3Clock localOutputClocks[1];

    // In this example we only check for ticking output clocks.
    if (!clocksTicked) {
        logEvent("No clock active in intermediateUpdate callback");
        return;
    }

    // In order to be threadsafe we have to check the clocks under lock
    // and copy their states
    GlobalLock(globalLockVar);
    if (!checkOutputClocks(S) & !checkCountdownClocks(S)) {
        GlobalUnlock(globalLockVar);
        logEvent("No clock active in intermediateUpdate callback");
        return;
    }
    localOutputClocks[0] = outputClocks[0];
    GlobalUnlock(globalLockVar);

    if (countdownClocksQualifier[0] == fmi3IntervalChanged) {
        // create a thread that will run the model partition for input clock 3
        logEvent("cb_intermediateUpdate starting thread for input clock 3 (vr=%d)", vr_inClock3);

        iu_arguments.S = S;
        //iu_arguments.s = s;
        iu_arguments.clockRef = vr_inClock3;
        iu_arguments.retval = &returnval;
        iu_arguments.activationTime = time;
        iu_arguments.delay = countdownClockIntervals[0];
        thr_handle = (HANDLE)_beginthreadex(NULL, 0, thr_activateModelPartition, &iu_arguments, 0, NULL);
        if (thr_handle != NULL) {
            // Bind the thread to the designated processor
            SetThreadAffinityMask(thr_handle, processorMask);
            // Set the designated priority
            SetThreadPriority(thr_handle, inputClockPrio[IDX_IN_CLOCK3]);
        }
        else {
            logEvent("Could not create thread in cb_intermediateUpdate");
        }

    }
    if (localOutputClocks[IDX_OUT_CLOCK]) {
        //  so far, no other action but a log message
        logEvent("Detected ticking of output clock (time=%g)", time);
    }
}
/*
 * cb_lockPreemption()
 * Callback function to grab a lock in order to avoid preemption in a critical section
 * Works on a globally defined variable initialized by the importer
*/
void cb_lockPreemption() {
    GlobalLock(globalLockVar);
}
/*
 * cb_unlockPreemption()
 * Callback function to release a lock
 * Works on a globally defined variable initialized by the importer
*/
void cb_unlockPreemption() {
    GlobalUnlock(globalLockVar);
}


/*
 * setAndCheckInputClocks()
 *
 * Function to set and determine the state of all *independent* Input clocks.
 * This depends on the current time (within the simulation) only
 * returns true, if any of the independent input clocks are set and thus a model partition must be activated
 */
static bool setAndCheckInputClocks(fmi3Float64 time) {
    // InClock_1: active every second
    if ((int)time % 1 == 0) {
        inputClocks[IDX_IN_CLOCK1] = fmi3ClockActive;
    }
    else {
        inputClocks[IDX_IN_CLOCK1] = fmi3ClockInactive;
    }
    // InClock_2: active at 0, 1, 8 and 9
    if (((int)time % 8 == 0) || ((int)time - 1) % 8 == 0) {
        inputClocks[IDX_IN_CLOCK2] = fmi3ClockActive;
    }
    else {
        inputClocks[IDX_IN_CLOCK2] = fmi3ClockInactive;
    }

    bool retval = inputClocks[IDX_IN_CLOCK1] == fmi3ClockActive || inputClocks[IDX_IN_CLOCK2] == fmi3ClockActive;
    if (retval) {
        logEvent("setAndCheckInputClocks: time = %d, inputClocks[InClock1] = %d, inputClocks[InClock2] = %d",
            (int)time, inputClocks[IDX_IN_CLOCK1], inputClocks[IDX_IN_CLOCK2]);
    }
    return retval;
}

/*
 * This function retrieves the state of the output clock of all partitions of the FMU
 * outputClocks[] is set accordingly
 * returns true if any of the outputClocks is actually set
 */
static bool checkOutputClocks(FMIInstance *S) {
    outputClocks[0] = fmi3ClockInactive;
    // TODO: handle status
    FMI3GetClock(S, vrOutputClocks, 1, outputClocks, 1);
    return outputClocks[0];
}

/*
 * This function retrieves the state of the countdown clock of all partitions of the FMU
 * outputClocks[] is set accordingly
 * returns true if interval changed set
 */
static bool checkCountdownClocks(FMIInstance *S) {
    countdownClockIntervals[0] = 0.0;
    countdownClocksQualifier[0] = fmi3IntervalNotYetKnown;
    // TODO: handle status
    FMI3GetIntervalDecimal(S, vrCountdownClocks, 1, countdownClockIntervals, countdownClocksQualifier, 1);
    return countdownClocksQualifier[0] == fmi3IntervalChanged;
}

/*
 * thr_activateModelPartition(args)
 *
 * called within a thread creation, whenever a model-partition is due to run
 * argument is an opaque pointer pointing to the necessary thread arguments => comp, clockRef, activationTime
 *
 * 1. the inputs of the respective model partition are set
 * 2. framework function fmi3ActivateModelPartition is called
 * 3. all outputs of the respective model partition are retrieved
 * 4. the outputs are recorded
 * 5. finally, the thread is ended, returning  fmi3Status (fmi3OK or fmi3Error)
 */
unsigned __stdcall thr_activateModelPartition(void *args)  {

    ThreadArgs* TA = (ThreadArgs *)args;

    //FMU *S = TA->S;
    //fmi3Instance s = TA->s;

    fmi3Status retval = fmi3OK;

    // Set Variables of appropriate partition
    switch (TA->clockRef) {
        case vr_inClock1: {
            logEvent("activateModelPartition calling fmi3ActivateModelPartition (%d)", TA->clockRef);
            // No variables to set for this partition
            retval = FMI3ActivateModelPartition(S, TA->clockRef, 0, TA->activationTime);
            if (retval != fmi3OK)    break;
            retval = FMI3GetInt32(S, vrOutputs_c1, 2, &outputs_c1[0], 2);
            recordVariables2(S->component, TA->activationTime, vr_inClock1);
        }
        break;
        case vr_inClock2: {
            logEvent("activateModelPartition calling fmi3ActivateModelPartition (%d)", TA->clockRef);
            retval = FMI3SetInt32(S, vrInputs_c2, 1, inputs_c2, 1);
            // Reset the source for the input again, so it is counted just once
            inputs_c2[0] = 0;
            if (retval != fmi3OK) break;
            retval = FMI3ActivateModelPartition(S, TA->clockRef, 0, TA->activationTime);
            if (retval != fmi3OK)    break;
            retval = FMI3GetInt32(S, vrOutputs_c2, 3, outputs_c2, 3);
            recordVariables2(S->component, TA->activationTime, vr_inClock2);
        }
        break;
        case vr_inClock3: {
            logEvent("activateModelPartition calling fmi3ActivateModelPartition (%d)", TA->clockRef);
            // No variables to set for this partition
            retval = FMI3ActivateModelPartition(S, TA->clockRef, 0, TA->activationTime);
            if (retval != fmi3OK)    break;
            retval = FMI3GetInt32(S, vrOutputs_c3, 3, outputs_c3, 3);
            // Use the output of model part 3 as input for model part 2
            inputs_c2[0] = outputs_c3[2];
            recordVariables2(S->component, TA->activationTime, vr_inClock3);
        }
        break;
        default:
            retval = fmi3Error;
    }
    _endthreadex(retval);
    return 0;
}
