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
#define FMI3_FUNCTION_PREFIX Controller_
#include "fmi3Functions.h"
#undef FMI3_FUNCTION_PREFIX

//#undef fmi3Functions_h
//#undef FMI3_FUNCTION_PREFIX
//#define FMI3_FUNCTION_PREFIX Plant_
//#include "fmi3Functions.h"
//#undef FMI3_FUNCTION_PREFIX

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
#define Controller_NDX 1
#define Controller_NZ 0
#define Controller_NU 1

// Controller vrefs
#define Controller_UR_ref 3
#define Controller_XR_ref 2

// Instance data sizing
#define MAX_NX 1
#define MAX_NZ 1
#define MAX_NU 1

// instance IDs
#define PLANT_ID 0
#define CONTROLLER_ID 1

#define N_INSTANCES 2

#define FIXED_STEP 1e-2
#define STOP_TIME 10

typedef struct
{
    fmi3Boolean timeEvent, stateEvent, enterEventMode, terminateSimulation, initialEventMode;
    fmi3Float64 x[MAX_NX];
    fmi3Float64 der_x[MAX_NX];
    fmi3Int32 rootsFound[MAX_NZ];
    fmi3Float64 u[MAX_NU];
} InstanceData;


#define OUTPUT_FILE_HEADER "time,x,r,x_r,u_r\n"
fmi3Status recordVariables(FILE *outputFile, fmi3Instance instances[], char *names[], fmi3Float64 time)
{
    const fmi3ValueReference plant_vref[Plant_NX] = {Plant_X_ref};
    fmi3Float64 plant_vals[Plant_NX] = {0};
    //Plant_fmi3GetFloat64(instances[PLANT_ID], plant_vref, Plant_NX, plant_vals, Plant_NX);

    const fmi3ValueReference controller_vref[Controller_NDX + Controller_NU] = {Controller_XR_ref, Controller_UR_ref};
    fmi3Float64 controller_vals[Controller_NDX + Controller_NU] = {0.0, 0.0};
    Controller_fmi3GetFloat64(instances[CONTROLLER_ID], controller_vref, Controller_NDX + Controller_NU, controller_vals, Controller_NDX + Controller_NU);

    //                                      time,     x,         r, x_r,                u_r
    fprintf(outputFile, "%g,%g,%d,%g,%g\n", time, plant_vals[0], 0, controller_vals[0], controller_vals[1]);
    return fmi3OK;
}

void initialize_instance_data(InstanceData data[])
{
    // Initialize instance data
    for (int i = 0; i < N_INSTANCES; i++)
    {
        data[i].timeEvent = fmi3False;
        data[i].stateEvent = fmi3False;
        data[i].enterEventMode = fmi3False;
        data[i].terminateSimulation = fmi3False;
        data[i].initialEventMode = fmi3True;
    }
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

fmi3Status initialize_all(fmi3Instance instances[], fmi3Float64 tStart, fmi3Float64 tEnd, char *names[],
                          fmi3EnterInitializationModeTYPE *enterInit[],
                          fmi3ExitInitializationModeTYPE *exitInit[])
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

    // Instances
    fmi3Instance instances[N_INSTANCES] = {NULL}; // Remaining elements are implicitly NULL

    char* names[N_INSTANCES] = { "plant", "controller" };

    // Instance functions
    fmi3InstantiateModelExchangeTYPE *instantiate[N_INSTANCES] = {
        NULL, //Plant_fmi3InstantiateModelExchange,
        Controller_fmi3InstantiateModelExchange};
    fmi3EnterInitializationModeTYPE *enterInit[N_INSTANCES] = {
        NULL, //Plant_fmi3EnterInitializationMode,
        Controller_fmi3EnterInitializationMode
    };
    fmi3ExitInitializationModeTYPE *exitInit[N_INSTANCES] = {
        NULL, //Plant_fmi3ExitInitializationMode,
        Controller_fmi3ExitInitializationMode
    };
    fmi3EnterConfigurationModeTYPE *enter_CT_mode[N_INSTANCES] = {
        NULL, //Plant_fmi3EnterConfigurationMode,
        Controller_fmi3EnterConfigurationMode
    };
    fmi3TerminateTYPE *terminate[N_INSTANCES] = {
        NULL, //Plant_fmi3Terminate,
        Controller_fmi3Terminate
    };
    fmi3FreeInstanceTYPE *freeInstance[N_INSTANCES] = {
        NULL, //Plant_fmi3FreeInstance,
        Controller_fmi3FreeInstance
    };

    // Instance refs
    const fmi3ValueReference plant_u_refs[Plant_NU] = {Plant_U_ref};

    // Recording
    FILE *outputFile = NULL;

    InstanceData plantD[N_INSTANCES];
    initialize_instance_data(plantD);

    outputFile = fopen("synchronous_control_me_out.csv", "w");
    if (!outputFile)
    {
        puts("Failed to open output file.");
        return EXIT_FAILURE;
    }

    fputs(OUTPUT_FILE_HEADER, outputFile);

    // Instantiate
    CHECK_STATUS(instantiate_all(instances, names, instantiate))

    // Initialize
    CHECK_STATUS(initialize_all(instances, tStart, tEnd, names, enterInit, exitInit))

    time = tStart;

    CHECK_STATUS(enter_CT_mode_all(instances, names, enter_CT_mode))

    // Record initial outputs
    recordVariables(outputFile, instances, names, time);
    
    // while (!plantD.terminateSimulation) {

    //     tNext = time + h;

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

    //         // enter Continuous-Time Mode
    //         CHECK_STATUS(Plant_fmi3EnterContinuousTimeMode(plant));

    //         // retrieve solution at simulation (re)start
    //         CHECK_STATUS(recordVariables(outputFile, plant, time));

    //         if (plantD.initialEventMode || valuesOfContinuousStatesChanged) {
    //             // the model signals a value change of states, retrieve them
    //             CHECK_STATUS(Plant_fmi3GetContinuousStates(plant, plantD.x, Plant_NX));
    //         }

    //         if (nextEventTimeDefined) {
    //             tNext = min(nextEventTime, tEnd);
    //         }
    //         else {
    //             tNext = tEnd;
    //         }

    //         plantD.initialEventMode = fmi3False;
    //     }

    //     if (time >= tEnd) {
    //         goto TERMINATE;
    //     }

    //     // compute derivatives
    //     CHECK_STATUS(Plant_fmi3GetContinuousStateDerivatives(plant, plantD.der_x, Plant_NX));

    //     // advance time
    //     h = min(fixedStep, tNext - time);
    //     time += h;
    //     CHECK_STATUS(Plant_fmi3SetTime(plant, time));

    //     // set continuous inputs at t = time
    //     Plant_fmi3SetFloat64(plant, plant_u_refs, Plant_NU, plantD.u, Plant_NU);

    //     // set states at t = time and perform one step
    //     for (size_t i = 0; i < Plant_NX; i++) {
    //         plantD.x[i] += h * plantD.der_x[i]; // forward Euler method
    //     }

    //     CHECK_STATUS(Plant_fmi3SetContinuousStates(plant, plantD.x, Plant_NX));

    //     // inform the model about an accepted step
    //     CHECK_STATUS(Plant_fmi3CompletedIntegratorStep(plant, fmi3True, &(plantD.enterEventMode), &(plantD.terminateSimulation)));

    //     // get continuous output
    //     // Plant_fmi3GetFloat*(m, ...)
    //     CHECK_STATUS(recordVariables(outputFile, plant, time));
    // }

TERMINATE:

    status = terminate_all(instances, names, terminate);

    clean_all(instances, names, freeInstance);

    puts("Done!");
    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
