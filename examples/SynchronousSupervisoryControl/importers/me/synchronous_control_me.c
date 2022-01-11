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
#define FMI3_FUNCTION_PREFIX Plant_
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

#include "util.h"

#define INSTANTIATION_TOKEN "{00000000-0000-0000-0000-000000000000}"

// Plant model size
#define Plant_NX 1
#define Plant_NZ 0
#define Plant_NU 1

// Plant vrefs
#define Plant_U_ref 3
#define Plant_X_ref 1

// Controller model size
#define Controller_NX 0
#define Controller_NY 1
#define Controller_NDX 1
#define Controller_NZ 0
#define Controller_NU 1

// Controller vrefs
#define Controller_UR_ref 3
#define Controller_XR_ref 2
#define Controller_R_ref 1

// Instance data sizing
#define MAX_NX 1
#define MAX_NZ 1
#define MAX_NU 1

// instance IDs
#define PLANT_ID 0
#define CONTROLLER_ID 1

#define N_INSTANCES 3

#define FIXED_STEP 1e-2
#define STOP_TIME 10

#define MAXDIRLENGTH 250

//typedef struct
//{
//    fmi3Boolean timeEvent, stateEvent, enterEventMode, terminateSimulation, initialEventMode;
//    fmi3Float64 x[MAX_NX];
//    fmi3Float64 der_x[MAX_NX];
//    fmi3Int32 rootsFound[MAX_NZ];
//    fmi3Float64 u[MAX_NU];
//} InstanceData;


#define OUTPUT_FILE_HEADER "time,x,r,x_r,u_r\n"
fmi3Status recordVariables(FILE *outputFile, fmi3Instance instances[], char *names[], fmi3Float64 time)
{
    const fmi3ValueReference plant_vref[Plant_NX] = {Plant_X_ref};
    fmi3Float64 plant_vals[Plant_NX] = {0};
    Plant_fmi3GetFloat64(instances[PLANT_ID], plant_vref, Plant_NX, plant_vals, Plant_NX);

    const fmi3ValueReference controller_vref[Controller_NDX + Controller_NU] = {Controller_XR_ref, Controller_UR_ref};
    fmi3Float64 controller_vals[Controller_NDX + Controller_NU] = {0.0, 0.0};
    Controller_fmi3GetFloat64(instances[CONTROLLER_ID], controller_vref, Controller_NDX + Controller_NU, controller_vals, Controller_NDX + Controller_NU);

    //                                      time,     x,         r, x_r,                u_r
    fprintf(outputFile, "%g,%g,%d,%g,%g\n", time, plant_vals[0], 0, controller_vals[0], controller_vals[1]);
    return fmi3OK;
}

fmi3Status instantiate_all(fmi3Instance instances[], char *names[], fmi3InstantiateModelExchangeTYPE *instantiate[])
{
    for (int i = 0; i < N_INSTANCES; i++)
    {

        printf("Instantiating %s... ", names[i]);

        if (instantiate[i]) {
            instances[i] = instantiate[i](names[i], INSTANTIATION_TOKEN, NULL, fmi3False, fmi3True, NULL, cb_logMessage);

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

fmi3Status enterInitAll(fmi3Instance instances[], fmi3Float64 tStart, fmi3Float64 tEnd, char* names[],
    fmi3EnterInitializationModeTYPE* enterInit[])
{
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

fmi3Status exitInitAll(fmi3Instance instances[], char* names[], fmi3ExitInitializationModeTYPE* exitInit[])
{
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

fmi3Status enter_CT_mode_all(fmi3Instance instances[], char *names[], fmi3EnterContinuousTimeModeTYPE *enter_CT_mode[])
{
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

fmi3Status terminate_all(fmi3Instance instances[], char *names[],
                          fmi3TerminateTYPE *terminate[])
{
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

void clean_all(fmi3Instance instances[], char *names[],
                          fmi3FreeInstanceTYPE *freeInstance[])
{
    for (int i = 0; i < N_INSTANCES; i++){
        if (instances[i]) {
            printf("Freeing %s... ", names[i]);
            freeInstance[i](instances[i]);
            printf("DONE. \n");
        }
    }
}

int main(int argc, char *argv[])
{
    printf("Running Supervisory Control example... \n");

    fmi3Status status = fmi3OK;
    const fmi3Float64 fixedStep = FIXED_STEP;
    fmi3Float64 h = fixedStep;
    fmi3Float64 tNext = h;
    const fmi3Float64 tEnd = STOP_TIME;
    fmi3Float64 time = 0;
    const fmi3Float64 tStart = 0;
    char cwd[MAXDIRLENGTH];

    // FMU data
    fmi3Float64 aux = 0;


    // Instances
    fmi3Instance instances[N_INSTANCES] = {NULL}; // Remaining elements are implicitly NULL

    char* names[N_INSTANCES] = { "plant", "controller", "supervisor"};

    // Instance functions
	fmi3InstantiateModelExchangeTYPE* instantiate[N_INSTANCES] = { 
        Plant_fmi3InstantiateModelExchange, 
        Controller_fmi3InstantiateModelExchange, 
        Supervisor_fmi3InstantiateModelExchange 
    };
	fmi3EnterInitializationModeTYPE* enterInit[N_INSTANCES] = { 
        Plant_fmi3EnterInitializationMode,
        Controller_fmi3EnterInitializationMode,
        Supervisor_fmi3EnterInitializationMode
    };
	fmi3ExitInitializationModeTYPE* exitInit[N_INSTANCES] = { 
        Plant_fmi3ExitInitializationMode,
        Controller_fmi3ExitInitializationMode,
        Supervisor_fmi3ExitInitializationMode
    };
	fmi3EnterConfigurationModeTYPE* enter_CT_mode[N_INSTANCES] = { 
        Plant_fmi3EnterContinuousTimeMode,
        Controller_fmi3EnterContinuousTimeMode,
        Supervisor_fmi3EnterContinuousTimeMode
    };
	fmi3TerminateTYPE* terminate[N_INSTANCES] = { 
        Plant_fmi3Terminate,
        Controller_fmi3Terminate,
        Supervisor_fmi3Terminate
    };
	fmi3FreeInstanceTYPE* freeInstance[N_INSTANCES] = { 
        Plant_fmi3FreeInstance,
        Controller_fmi3FreeInstance,
        Supervisor_fmi3FreeInstance
    };

    // Instance refs
    const fmi3ValueReference plant_u_refs[Plant_NU] = { Plant_U_ref };
    const fmi3ValueReference plant_y_refs[Plant_NX] = { Plant_X_ref };
    const fmi3ValueReference controller_u_refs[Controller_NU] = { Controller_XR_ref };
    const fmi3ValueReference controller_y_refs[Controller_NY] = { Controller_UR_ref };
    const fmi3ValueReference controller_r_refs[] = { Controller_R_ref };
    
    // Will hold exchanged values: Controller -> Plant
    fmi3Float64 controller_vals[Controller_NY] = { 0.0 };
    fmi3Clock controller_r_vals[] = { fmi3ClockActive };
    // Will hold exchanged values: Plant -> Controller
    fmi3Float64 plant_vals[Plant_NX] = { 0.0 };
    fmi3Float64 plant_der_vals[Plant_NX] = { 0.0 };

    // Recording
    FILE *outputFile = NULL;
    
    // Controller's clock r timer
    fmi3Float64 controller_r_period = 1.0;
    fmi3Float64 controller_r_timer = controller_r_period;

    getcwd(cwd, MAXDIRLENGTH);
    puts("Opening output file in cwd:");
    printf("%s\n", cwd);

    outputFile = fopen("synchronous_control_me_out.csv", "w");
    if (!outputFile)
    {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    fputs(OUTPUT_FILE_HEADER, outputFile);

    // Instantiate
    CHECK_STATUS(instantiate_all(instances, names, instantiate))

    // Set debug logging
    char* categories[] = { "logEvents", "logStatusError" };
    Controller_fmi3SetDebugLogging(instances[CONTROLLER_ID], true, 2, categories);

    // Initialize
    CHECK_STATUS(enterInitAll(instances, tStart, tEnd, names, enterInit))

    // Exchange data Controller -> Plant
    Controller_fmi3GetFloat64(instances[CONTROLLER_ID], controller_y_refs, Controller_NY, controller_vals, Controller_NY);
    Plant_fmi3SetFloat64(instances[PLANT_ID], plant_u_refs, Plant_NU, controller_vals, Plant_NU);

    //Exchange data Plant -> Controller
    Plant_fmi3GetFloat64(instances[PLANT_ID], plant_y_refs, Plant_NX, plant_vals, Plant_NX);
    Controller_fmi3SetFloat64(instances[CONTROLLER_ID], controller_u_refs, Controller_NU, plant_vals, Controller_NU);

    CHECK_STATUS(exitInitAll(instances, names, exitInit))
    
    time = tStart;

    CHECK_STATUS(enter_CT_mode_all(instances, names, enter_CT_mode))

    // Record initial outputs
    recordVariables(outputFile, instances, names, time);

    while (time + h <= tEnd)
    {
        // Advance time and update timers
        time += h;
        controller_r_timer -= h;

        // Check if controller needs to execute
        if (controller_r_timer <= 0.0) {

            printf("Entering event mode for ticking clock r. \n");

            // Reset timer
            controller_r_timer = controller_r_period;

            // Put Controller into event mode, as clocks are about to tick
            CHECK_STATUS(Controller_fmi3EnterEventMode(instances[CONTROLLER_ID], fmi3False, fmi3False, NULL, 0, fmi3False));
            
            // Activate clock
            CHECK_STATUS(Controller_fmi3SetClock(instances[CONTROLLER_ID], controller_r_refs, 1, controller_r_vals, 1));

            // Set inputs to clocked partition: Exchange data Plant -> Controller
            Plant_fmi3GetFloat64(instances[PLANT_ID], plant_y_refs, Plant_NX, plant_vals, Plant_NX);
            Controller_fmi3SetFloat64(instances[CONTROLLER_ID], controller_u_refs, Controller_NU, plant_vals, Controller_NU);

            // Compute outputs to clocked partition: Exchange data Controller -> Plant
            Controller_fmi3GetFloat64(instances[CONTROLLER_ID], controller_y_refs, Controller_NY, controller_vals, Controller_NY);
            Plant_fmi3SetFloat64(instances[PLANT_ID], plant_u_refs, Plant_NU, controller_vals, Plant_NU);
            
            // Update discrete states of the controller
            fmi3Boolean nominalsChanged = fmi3False;
            fmi3Boolean statesChanged = fmi3False;
            fmi3Boolean nextEventTimeDefined = fmi3False;
            fmi3Boolean terminateSimulation = fmi3False;
            fmi3Boolean discreteStatesNeedUpdate = fmi3False;
            fmi3Float64 nextEventTime = INFINITY;
            Controller_fmi3UpdateDiscreteStates(instances[CONTROLLER_ID], &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime);

            // Exit event mode
            Controller_fmi3EnterContinuousTimeMode(instances[CONTROLLER_ID]);
            printf("Exiting event mode. \n");
        }

        // Estimate next Plant state
        Plant_fmi3GetContinuousStates(instances[PLANT_ID], plant_vals, Plant_NX);
        Plant_fmi3GetContinuousStateDerivatives(instances[PLANT_ID], plant_der_vals, Plant_NX);
        plant_vals[0] += h * plant_der_vals[0];

        // Set FMU time
        CHECK_STATUS(Plant_fmi3SetTime(instances[PLANT_ID], time));
        CHECK_STATUS(Controller_fmi3SetTime(instances[CONTROLLER_ID], time));

        // Update Plant state
        Plant_fmi3SetContinuousStates(instances[PLANT_ID], plant_vals, Plant_NX);

        // Record data
        recordVariables(outputFile, instances, names, time);
    }

//     // handle events
//     if (plantD.enterEventMode || plantD.stateEvent || plantD.timeEvent) {

//         if (!plantD.initialEventMode) {
//             CHECK_STATUS(Plant_fmi3EnterEventMode(plant, plantD.enterEventMode, plantD.stateEvent, plantD.rootsFound, Plant_NZ, plantD.timeEvent));
//         }

//         // event iteration
//         fmi3Boolean discreteStatesNeedUpdate = fmi3True;
//         fmi3Boolean terminateSimulation = fmi3False;
//         fmi3Boolean nominalsOfContinuousStatesChanged = fmi3False;
//         fmi3Boolean valuesOfContinuousStatesChanged = fmi3False;
//         fmi3Boolean nextEventTimeDefined = fmi3False;
//         fmi3Float64 nextEventTime = 0;

//         while (discreteStatesNeedUpdate) {

//             // set inputs at super dense time point
//             // Plant_fmi3SetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

//             fmi3Boolean nominalsChanged = fmi3False;
//             fmi3Boolean statesChanged = fmi3False;

//             // update discrete states
//             CHECK_STATUS(Plant_fmi3UpdateDiscreteStates(plant, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

//             // getOutput at super dense time point
//             // Plant_fmi3GetFloat*/Int*/UInt*/Boolean/String/Binary(m, ...)

//             nominalsOfContinuousStatesChanged |= nominalsChanged;
//             valuesOfContinuousStatesChanged |= statesChanged;

//             if (terminateSimulation) goto TERMINATE;
//         }

TERMINATE:

    status = terminate_all(instances, names, terminate);

    clean_all(instances, names, freeInstance);

    fclose(outputFile);

    puts("Done!");
    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
