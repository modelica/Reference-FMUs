#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#undef fmi3Functions_h
#undef FMI3_FUNCTION_PREFIX
#define FMI3_FUNCTION_PREFIX Plantmodel_
#include "fmi3Functions.h"
#undef FMI3_FUNCTION_PREFIX

#undef fmi3Functions_h
#undef FMI3_FUNCTION_PREFIX
#define FMI3_FUNCTION_PREFIX Controller_
#include "fmi3Functions.h"
#undef FMI3_FUNCTION_PREFIX

#undef fmi3Functions_h
#undef FMI3_FUNCTION_PREFIX
#define FMI3_FUNCTION_PREFIX Supervisor_
#include "fmi3Functions.h"
#undef FMI3_FUNCTION_PREFIX

#define INSTANTIATION_TOKEN "{00000000-0000-0000-0000-000000000000}"

// Plantmodel vrefs
#define Plantmodel_U_ref  3
#define Plantmodel_X_ref  1

// Controller vrefs
#define Controller_UR_ref 3
#define Controller_XR_ref 2
#define Controller_R_ref  1
#define Controller_S_ref  6
#define Controller_AS_ref 5

// Supervisor vrefs
#define Supervisor_S_ref  1
#define Supervisor_X_ref  2
#define Supervisor_AS_ref 3

// instance IDs
#define PLANTMODEL_ID 0
#define CONTROLLER_ID 1
#define SUPERVISOR_ID 2

#define N_INSTANCES 3

#define FIXED_STEP 1e-2
#define STOP_TIME 10.0

#define MAXDIRLENGTH 250

// Instance names
const char* names[N_INSTANCES] = { "plantmodel", "controller", "supervisor" };

// Constants with value references
const fmi3ValueReference plantmodel_u_refs[]  = { Plantmodel_U_ref  };
const fmi3ValueReference plantmodel_y_refs[]  = { Plantmodel_X_ref  };
const fmi3ValueReference controller_u_refs[]  = { Controller_XR_ref };
const fmi3ValueReference controller_y_refs[]  = { Controller_UR_ref };
const fmi3ValueReference controller_r_refs[]  = { Controller_R_ref  };
const fmi3ValueReference controller_s_refs[]  = { Controller_S_ref  };
const fmi3ValueReference controller_as_refs[] = { Controller_AS_ref };
const fmi3ValueReference supervisor_s_refs[]  = { Supervisor_S_ref  };
const fmi3ValueReference supervisor_as_refs[] = { Supervisor_AS_ref };
const fmi3ValueReference supervisor_in_refs[] = { Supervisor_X_ref  };

// Simulation constants
const fmi3Float64 tEnd = STOP_TIME;
const fmi3Float64 tStart = 0;


static void cb_logMessage3(fmi3InstanceEnvironment instanceEnvironment,
    fmi3String instanceName,
    fmi3Status status,
    fmi3String category,
    fmi3String message) {
    printf("[%s] - %s: %s\n", instanceName, category, message);
}


//**************** Output aux functions ******************//

#define OUTPUT_FILE_HEADER "time,x,r,x_r,u_r\n"

fmi3Status recordVariables(FILE *outputFile, fmi3Instance instances[], fmi3Float64 time)
{
    const fmi3ValueReference plantmodel_vref[] = {Plantmodel_X_ref};
    fmi3Float64 plantmodel_vals[] = {0};
    Plantmodel_fmi3GetFloat64(instances[PLANTMODEL_ID], plantmodel_vref, 1, plantmodel_vals, 1);

    const fmi3ValueReference controller_vref[] = {Controller_XR_ref, Controller_UR_ref};
    fmi3Float64 controller_vals[] = {0.0, 0.0};
    Controller_fmi3GetFloat64(instances[CONTROLLER_ID], controller_vref, 2, controller_vals, 2);

    //                                      time,     x,         r, x_r,                u_r
    fprintf(outputFile, "%g,%g,%d,%g,%g\n", time, plantmodel_vals[0], 0, controller_vals[0], controller_vals[1]);
    return fmi3OK;
}

FILE* initializeFile(char* fname) {
    FILE* outputFile = fopen(fname, "w");
    if (!outputFile)
    {
        puts("Failed to open output file.");
        return NULL;
    }
    fputs(OUTPUT_FILE_HEADER, outputFile);
    return outputFile;
}

//*******************************************************//



fmi3Status instantiateAll(fmi3Instance instances[])
{
    fmi3InstantiateModelExchangeTYPE* instantiate[N_INSTANCES] = {
        Plantmodel_fmi3InstantiateModelExchange,
        Controller_fmi3InstantiateModelExchange,
        Supervisor_fmi3InstantiateModelExchange
    };

    for (int i = 0; i < N_INSTANCES; i++)
    {

        printf("Instantiating %s... ", names[i]);

        if (instantiate[i]) {
            instances[i] = instantiate[i](names[i], INSTANTIATION_TOKEN, NULL, fmi3False, fmi3True, NULL, cb_logMessage3);

            if (instances[i] == NULL)
            {
                printf("Failed to instantiate %s. \n", names[i]);
                return fmi3Fatal;
            }
        }

        printf("DONE. \n");
    }
    return fmi3OK;
}

fmi3Status enterInitializationModeAll(fmi3Instance instances[])
{
    fmi3EnterInitializationModeTYPE* enterInit[N_INSTANCES] = {
        Plantmodel_fmi3EnterInitializationMode,
        Controller_fmi3EnterInitializationMode,
        Supervisor_fmi3EnterInitializationMode
    };

    fmi3Status status = fmi3OK;

    // Enter init mode
    for (int i = 0; i < N_INSTANCES; i++)
    {
        printf("Entering init mode for %s... ", names[i]);

        if (enterInit[i]) {
            status = enterInit[i](instances[i], fmi3False, 0.0, tStart, fmi3True, tEnd);
            if (status != fmi3OK)
            {
                printf("Failed to enter init mode for %s. \n", names[i]);
                return fmi3Fatal;
            }
        }

        printf("DONE. \n");
    }

    return fmi3OK;
}

fmi3Status exitInitializationModeAll(fmi3Instance instances[])
{
    fmi3ExitInitializationModeTYPE* exitInit[N_INSTANCES] = {
        Plantmodel_fmi3ExitInitializationMode,
        Controller_fmi3ExitInitializationMode,
        Supervisor_fmi3ExitInitializationMode
    };

    fmi3Status status = fmi3OK;

    for (int i = 0; i < N_INSTANCES; i++)
    {
        printf("Exiting init mode for %s... ", names[i]);

        if (exitInit[i]) {
            status = exitInit[i](instances[i]);
            if (status != fmi3OK)
            {
                printf("Failed to enter init mode for %s. \n", names[i]);
                return status;
            }
        }

        printf("DONE. \n");
    }

    return fmi3OK;
}

fmi3Status enterContinuousTimeModeAll(fmi3Instance instances[])
{
    fmi3EnterConfigurationModeTYPE* enter_CT_mode[N_INSTANCES] = {
        Plantmodel_fmi3EnterContinuousTimeMode,
        Controller_fmi3EnterContinuousTimeMode,
        Supervisor_fmi3EnterContinuousTimeMode
    };

    fmi3Status status = fmi3OK;

    for (int i = 0; i < N_INSTANCES; i++)
    {
        printf("Entering CT mode for %s... ", names[i]);

        if (enter_CT_mode[i]) {
            status = enter_CT_mode[i](instances[i]);
            if (status != fmi3OK)
            {
                printf("Failed to enter CT mode for %s. \n", names[i]);
                return fmi3Fatal;
            }
        }

        printf("DONE. \n");
    }

    return fmi3OK;
}

fmi3Status terminateAll(fmi3Instance instances[])
{
    fmi3TerminateTYPE* terminate[N_INSTANCES] = {
        Plantmodel_fmi3Terminate,
        Controller_fmi3Terminate,
        Supervisor_fmi3Terminate
    };

    fmi3Status status = fmi3OK;

    for (int i = 0; i < N_INSTANCES; i++) {
        if (instances[i] && status != fmi3Error && status != fmi3Fatal) {
            // terminate simulation
            printf("Terminating %s... ", names[i]);
            fmi3Status s = terminate[i](instances[i]);
            printf("DONE. \n");
            status = max(status, s);
        }
    }

    return status;
}

void freeInstanceAll(fmi3Instance instances[])
{
    fmi3FreeInstanceTYPE* freeInstance[N_INSTANCES] = {
        Plantmodel_fmi3FreeInstance,
        Controller_fmi3FreeInstance,
        Supervisor_fmi3FreeInstance
    };

    for (int i = 0; i < N_INSTANCES; i++){
        if (instances[i]) {
            printf("Freeing %s... ", names[i]);
            freeInstance[i](instances[i]);
            printf("DONE. \n");
        }
    }
}

void handleTimeEventController(fmi3Instance instances[]) {
    // Activate Controller's clock r
    fmi3Clock controller_r_vals[] = { fmi3ClockActive };
    Controller_fmi3SetClock(instances[CONTROLLER_ID], controller_r_refs, 1, controller_r_vals);
    
    // Set inputs to clocked partition: Exchange data Plantmodel -> Controller
    fmi3Float64 plantmodel_vals[] = { 0.0 };
    Plantmodel_fmi3GetFloat64(instances[PLANTMODEL_ID], plantmodel_y_refs, 1, plantmodel_vals, 1);
    Controller_fmi3SetFloat64(instances[CONTROLLER_ID], controller_u_refs, 1, plantmodel_vals, 1);
    
    // Compute outputs to clocked partition: Exchange data Controller -> Plantmodel
    fmi3Float64 controller_vals[] = { 0.0 };
    Controller_fmi3GetFloat64(instances[CONTROLLER_ID], controller_y_refs, 1, controller_vals, 1);
    Plantmodel_fmi3SetFloat64(instances[PLANTMODEL_ID], plantmodel_u_refs, 1, controller_vals, 1);
    
}

void handleStateEventSupervisor(fmi3Instance instances[]) {
    fmi3Clock supervisor_s_vals[] = { fmi3ClockInactive };
    fmi3Float64 supervisor_as_vals[] = { 0.0 };

    // Propagate clock activation Supervisor -> Controller
    Supervisor_fmi3GetClock(instances[SUPERVISOR_ID], supervisor_s_refs, 1, supervisor_s_vals);
    assert(supervisor_s_vals[0] == fmi3ClockActive);
    Controller_fmi3SetClock(instances[CONTROLLER_ID], controller_s_refs, 1, supervisor_s_vals);

    // Exchange data Supervisor -> Controller
    Supervisor_fmi3GetFloat64(instances[SUPERVISOR_ID], supervisor_as_refs, 1, supervisor_as_vals, 1);
    Controller_fmi3SetFloat64(instances[CONTROLLER_ID], controller_as_refs, 1, supervisor_as_vals, 1);
}

int main(int argc, char *argv[])
{
    printf("Running Supervisory Control example... \n");

    fmi3Status status = fmi3OK;
    fmi3Float64 h = FIXED_STEP;
    fmi3Float64 tNext = h;
    fmi3Float64 time = 0;
    
    // Instances
    fmi3Instance instances[N_INSTANCES] = {NULL}; // Remaining elements are implicitly NULL
    
    // Will hold exchanged values: Controller -> Plantmodel
    fmi3Float64 controller_vals[] = { 0.0 };
    // Will hold exchanged values: Plantmodel -> Controller
    fmi3Float64 plantmodel_vals[] = { 0.0 };
    fmi3Float64 plantmodel_der_vals[] = { 0.0 };
    // Will hold event indicator values of supervisor;
    fmi3Float64 supervisor_evt_vals[1] = { 0.0 };
    fmi3Float64 supervisor_event_indicator = 0.0;
    
    // Controller's clock r timer
    fmi3Float64 controller_r_period = 0.1;
    fmi3Float64 controller_r_timer = controller_r_period;

    // Open file
    FILE * outputFile = initializeFile("synchronous_control_me_out.csv");
    if (!outputFile) {
        return EXIT_FAILURE;
    }

    // Instantiate
    instantiateAll(instances);

    // Set debug logging
    char* categories[] = { "logEvents", "logStatusError" };
    Controller_fmi3SetDebugLogging(instances[CONTROLLER_ID], true, 2, categories);
    Supervisor_fmi3SetDebugLogging(instances[SUPERVISOR_ID], true, 2, categories);
    Plantmodel_fmi3SetDebugLogging(instances[PLANTMODEL_ID], true, 2, categories);

    // Initialize
    enterInitializationModeAll(instances);

    // Exchange data Controller -> Plantmodel
    Controller_fmi3GetFloat64(instances[CONTROLLER_ID], controller_y_refs, 1, controller_vals, 1);
    Plantmodel_fmi3SetFloat64(instances[PLANTMODEL_ID], plantmodel_u_refs, 1, controller_vals, 1);

    //Exchange data Plantmodel -> Controller
    Plantmodel_fmi3GetFloat64(instances[PLANTMODEL_ID], plantmodel_y_refs, 1, plantmodel_vals, 1);
    Controller_fmi3SetFloat64(instances[CONTROLLER_ID], controller_u_refs, 1, plantmodel_vals, 1);

    //Exchange data Plantmodel -> Supervisor
    Supervisor_fmi3SetFloat64(instances[SUPERVISOR_ID], supervisor_in_refs, 1, plantmodel_vals, 1);

    // Initialize event indicators
    Supervisor_fmi3GetEventIndicators(instances[SUPERVISOR_ID], supervisor_evt_vals, 1);
    supervisor_event_indicator = supervisor_evt_vals[0];

    exitInitializationModeAll(instances);
    
    time = tStart;

    enterContinuousTimeModeAll(instances);

    // Record initial outputs
    recordVariables(outputFile, instances, time);

    while (time + h <= tEnd) {
        // Advance time and update timers
        time += h;
        controller_r_timer -= h;
        
        // Check for state events or time events.
        bool timeEvent = controller_r_timer <= 0.0;
        
        Supervisor_fmi3GetEventIndicators(instances[SUPERVISOR_ID], supervisor_evt_vals, 1);
        bool stateEvent = supervisor_event_indicator * supervisor_evt_vals[0] < 0.0;
        supervisor_event_indicator = supervisor_evt_vals[0];

        printf("Time event: %d \t State Event: %d \n", timeEvent, stateEvent);

        // Check if controller needs to execute
        if (timeEvent || stateEvent) {
            if (timeEvent && !stateEvent) {
                printf("Entering event mode for ticking clock r. \n");

                // Reset timer
                controller_r_timer = controller_r_period;

                // Put Controller into event mode, as clocks are about to tick
                Controller_fmi3EnterEventMode(instances[CONTROLLER_ID], fmi3False, fmi3False, NULL, 0, fmi3True);
                // Put Plantmodel into event mode, as its input is a discrete time variable
                Plantmodel_fmi3EnterEventMode(instances[PLANTMODEL_ID], fmi3False, fmi3False, NULL, 0, fmi3False);

                // Handle time event in controller
                handleTimeEventController(instances);

                // Update discrete states of the controller
                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;
                fmi3Boolean nextEventTimeDefined = fmi3False;
                fmi3Boolean terminateSimulation = fmi3False;
                fmi3Boolean discreteStatesNeedUpdate = fmi3False;
                fmi3Float64 nextEventTime = INFINITY;
                Controller_fmi3UpdateDiscreteStates(instances[CONTROLLER_ID], &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime);
                Plantmodel_fmi3UpdateDiscreteStates(instances[PLANTMODEL_ID], &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime);

                // Exit event mode
                Controller_fmi3EnterContinuousTimeMode(instances[CONTROLLER_ID]);
                Plantmodel_fmi3EnterContinuousTimeMode(instances[PLANTMODEL_ID]);
                printf("Exiting event mode. \n");
            }
            else if (!timeEvent && stateEvent) {
                printf("Entering event mode for ticking clock s. \n");

                // Put Supervisor and Controller into event mode, as clocks are about to tick. Note the flags used.
                Supervisor_fmi3EnterEventMode(instances[SUPERVISOR_ID], fmi3False, fmi3True, NULL, 0, fmi3False);
                Controller_fmi3EnterEventMode(instances[CONTROLLER_ID], fmi3False, fmi3False, NULL, 0, fmi3False);

                // Handle state event supervisor
                handleStateEventSupervisor(instances);

                // Update discrete states of the controller and supervisor
                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;
                fmi3Boolean nextEventTimeDefined = fmi3False;
                fmi3Boolean terminateSimulation = fmi3False;
                fmi3Boolean discreteStatesNeedUpdate = fmi3False;
                fmi3Float64 nextEventTime = INFINITY;
                Supervisor_fmi3UpdateDiscreteStates(instances[SUPERVISOR_ID], &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime);
                Controller_fmi3UpdateDiscreteStates(instances[CONTROLLER_ID], &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime);
                
                // Exit event mode
                Supervisor_fmi3EnterContinuousTimeMode(instances[SUPERVISOR_ID]);
                Controller_fmi3EnterContinuousTimeMode(instances[CONTROLLER_ID]);
                printf("Exiting event mode. \n");
            }
            else {
                assert(timeEvent && stateEvent);

                // Reset timer
                controller_r_timer = controller_r_period;

                // Handle both time event and state event.
                printf("Entering event mode for ticking clocks s and r. \n");
                // Now we must respect the dependencies. The supervisor gets priority, and then the controller.

                // Put Supervisor and Controller into event mode, as clocks are about to tick. Note the flags used.
                Supervisor_fmi3EnterEventMode(instances[SUPERVISOR_ID], fmi3False, fmi3True, NULL, 0, fmi3False);
                Controller_fmi3EnterEventMode(instances[CONTROLLER_ID], fmi3False, fmi3False, NULL, 0, fmi3True);
                // Put Plantmodel into event mode, as its input is a discrete time variable
                Plantmodel_fmi3EnterEventMode(instances[PLANTMODEL_ID], fmi3False, fmi3False, NULL, 0, fmi3False);

                handleStateEventSupervisor(instances);
                handleTimeEventController(instances);

                // Update discrete states of the controller and supervisor
                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;
                fmi3Boolean nextEventTimeDefined = fmi3False;
                fmi3Boolean terminateSimulation = fmi3False;
                fmi3Boolean discreteStatesNeedUpdate = fmi3False;
                fmi3Float64 nextEventTime = INFINITY;
                Supervisor_fmi3UpdateDiscreteStates(instances[SUPERVISOR_ID], &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime);
                Controller_fmi3UpdateDiscreteStates(instances[CONTROLLER_ID], &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime);
                Plantmodel_fmi3UpdateDiscreteStates(instances[PLANTMODEL_ID], &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime);

                // Exit event mode
                Supervisor_fmi3EnterContinuousTimeMode(instances[SUPERVISOR_ID]);
                Controller_fmi3EnterContinuousTimeMode(instances[CONTROLLER_ID]);
                Plantmodel_fmi3EnterContinuousTimeMode(instances[PLANTMODEL_ID]);
                printf("Exiting event mode. \n");
            }
        }

        // Exchange data Plantmodel -> Supervisor
        Plantmodel_fmi3GetFloat64(instances[PLANTMODEL_ID], plantmodel_y_refs, 1, plantmodel_vals, 1);
        Supervisor_fmi3SetFloat64(instances[SUPERVISOR_ID], supervisor_in_refs, 1, plantmodel_vals, 1);

        // Estimate next Plantmodel state
        Plantmodel_fmi3GetContinuousStates(instances[PLANTMODEL_ID], plantmodel_vals, 1);
        Plantmodel_fmi3GetContinuousStateDerivatives(instances[PLANTMODEL_ID], plantmodel_der_vals, 1);
        plantmodel_vals[0] += h * plantmodel_der_vals[0];

        // Set FMU time
        Plantmodel_fmi3SetTime(instances[PLANTMODEL_ID], time);
        Controller_fmi3SetTime(instances[CONTROLLER_ID], time);
        Supervisor_fmi3SetTime(instances[SUPERVISOR_ID], time);

        // Update Plantmodel state
        Plantmodel_fmi3SetContinuousStates(instances[PLANTMODEL_ID], plantmodel_vals, 1);

        // Record data
        recordVariables(outputFile, instances, time);
    }

    status = terminateAll(instances);

    freeInstanceAll(instances);

    fclose(outputFile);

    puts("Done!");
    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
