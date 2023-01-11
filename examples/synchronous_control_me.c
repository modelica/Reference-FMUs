/*
The example consists of a controller and a plant, and a supervisor FMU.
It uses model exchange in FMI3.0. The Controller fmu declares an input periodic clock,
and the supervisor has an output clock that triggers when a state event occurs.
The output clock of the supervisor is connected to another input clock of the controller.

See more details in [synchronous_control_me.md](./synchronous_control_me.md)
*/


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#include "FMI3.h"

// Plantmodel vrefs
#define Plantmodel_U_ref  3
#define Plantmodel_X_ref  1

// Controller vrefs
#define Controller_UR_ref 3
#define Controller_X_ref 2
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
const fmi3ValueReference controller_u_refs[]  = { Controller_X_ref };
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


static void logMessage(FMIInstance* instance, FMIStatus status, const char* category, const char* message) {

    switch (status) {
    case FMIOK:
        printf("[OK] ");
        break;
    case FMIWarning:
        printf("[Warning] ");
        break;
    case FMIDiscard:
        printf("[Discard] ");
        break;
    case FMIError:
        printf("[Error] ");
        break;
    case FMIFatal:
        printf("[Fatal] ");
        break;
    case FMIPending:
        printf("[Pending] ");
        break;
    }

    printf("[%s] %s\n", instance->name, message);
}


//**************** Output aux functions ******************//

#define OUTPUT_FILE_HEADER "time,x,r,x_r,u_r\n"

#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

static FMIStatus recordVariables(FILE *outputFile, FMIInstance* controller, FMIInstance* plant, fmi3Float64 time) {

    FMIStatus status = FMIOK;

    const fmi3ValueReference plantmodel_vref[] = {Plantmodel_X_ref};
    fmi3Float64 plantmodel_vals[] = {0};
    CALL(FMI3GetFloat64(plant, plantmodel_vref, 1, plantmodel_vals, 1));

    const fmi3ValueReference controller_vref[] = {Controller_X_ref, Controller_UR_ref};
    fmi3Float64 controller_vals[] = {0.0, 0.0};
    CALL(FMI3GetFloat64(controller, controller_vref, 2, controller_vals, 2));

    //                                      time,     x,         r, x_r,                u_r
    fprintf(outputFile, "%g,%g,%d,%g,%g\n", time, plantmodel_vals[0], 0, controller_vals[0], controller_vals[1]);

TERMINATE:
    return status;
}

FILE* initializeFile(char* fname) {
    FILE* outputFile = fopen(fname, "w");
    if (!outputFile) {
        puts("Failed to open output file.");
        return NULL;
    }
    fputs(OUTPUT_FILE_HEADER, outputFile);
    return outputFile;
}

//*******************************************************//

#if defined(_WIN32)
#define BINARY_DIR "\\binaries\\x86_64-windows\\"
#define BINARY_EXT ".dll"
#elif defined(__APPLE__)
#define BINARY_DIR "/binaries/x86_64-darwin/"
#define BINARY_EXT ".dylib"
#else
#define BINARY_DIR "/binaries/x86_64-linux/"
#define BINARY_EXT ".so"
#endif


static FMIStatus handleTimeEventController(FMIInstance* controller, FMIInstance* plant) {

    FMIStatus status = FMIOK;

    // Activate Controller's clock r
    fmi3Clock controller_r_vals[] = { fmi3ClockActive };
    CALL(FMI3SetClock(controller, controller_r_refs, 1, controller_r_vals));

    // Inputs to the clocked partition are assumed to have been set already, in the continuous time

    // Compute outputs to clocked partition: Exchange data Controller -> Plantmodel
    fmi3Float64 controller_vals[] = { 0.0 };
    CALL(FMI3GetFloat64(controller, controller_y_refs, 1, controller_vals, 1));
    CALL(FMI3SetFloat64(plant, plantmodel_u_refs, 1, controller_vals, 1));

TERMINATE:
    return status;
}

static FMIStatus handleStateEventSupervisor(FMIInstance* controller, FMIInstance* supervisor) {

    FMIStatus status = FMIOK;

    fmi3Clock supervisor_s_vals[] = { fmi3ClockInactive };
    fmi3Float64 supervisor_as_vals[] = { 0.0 };

    // Propagate clock activation Supervisor -> Controller
    CALL(FMI3GetClock(supervisor, supervisor_s_refs, 1, supervisor_s_vals));
    assert(supervisor_s_vals[0] == fmi3ClockActive);
    CALL(FMI3SetClock(controller, controller_s_refs, 1, supervisor_s_vals));

    // Exchange data Supervisor -> Controller
    CALL(FMI3GetFloat64(supervisor, supervisor_as_refs, 1, supervisor_as_vals, 1));
    CALL(FMI3SetFloat64(controller, controller_as_refs, 1, supervisor_as_vals, 1));

TERMINATE:
    return status;
}

int main(int argc, char *argv[])
{
    printf("Running Supervisory Control example... \n");

    FMIStatus status = FMIOK;
    fmi3Float64 h = FIXED_STEP;
    fmi3Float64 tNext = h;
    fmi3Float64 time = 0;

    // Will hold exchanged values: Controller -> Plantmodel
    fmi3Float64 controller_vals[] = { 0.0 };
    // Will hold exchanged values: Plantmodel -> Controller
    fmi3Float64 plantmodel_vals[] = { 0.0 };
    fmi3Float64 plantmodel_der_vals[] = { 0.0 };
    // Will hold event indicator values of supervisor;
    fmi3Float64 supervisor_evt_vals[1] = { 0.0 };
    fmi3Float64 supervisor_event_indicator = 0.0;

    // Controller's clock r timer
    fmi3Float64 controller_r_period = 0.0;
    fmi3Float64 controller_r_timer = 0.0;
    // Will hold output from FMI3GetIntervalDecimal
    fmi3Float64 controller_interval_vals[] = { 0.0 };
    fmi3IntervalQualifier controller_interval_qualifiers[] = { fmi3IntervalNotYetKnown };

    // Open file
    FILE * outputFile = initializeFile("synchronous_control_me_out.csv");
    if (!outputFile) {
        return EXIT_FAILURE;
    }

    // Instantiate
    FMIInstance* controller = FMICreateInstance("controller", "Controller" BINARY_DIR "Controller" BINARY_EXT, logMessage, NULL);
    FMIInstance* plant      = FMICreateInstance("plant",      "Plant"      BINARY_DIR "Plant"      BINARY_EXT, logMessage, NULL);
    FMIInstance* supervisor = FMICreateInstance("supervisor", "Supervisor" BINARY_DIR "Supervisor" BINARY_EXT, logMessage, NULL);

    if (!controller || !plant || !supervisor) {
        puts("Failed to load shared libraries.");
        return FMIError;
    }

    CALL(FMI3InstantiateModelExchange(controller, "{e1f14bf0-302d-4ef9-b11c-e01c7ed456cb}", NULL, fmi3False, fmi3False));
    CALL(FMI3InstantiateModelExchange(plant,      "{6e81b08d-97be-4de1-957f-8358a4e83184}", NULL, fmi3False, fmi3False));
    CALL(FMI3InstantiateModelExchange(supervisor, "{64202d14-799a-4379-9fb3-79354aec17b2}", NULL, fmi3False, fmi3False));

    // Initialize
    CALL(FMI3EnterInitializationMode(controller, fmi3False, 0.0, tStart, fmi3True, tEnd));
    CALL(FMI3EnterInitializationMode(plant,      fmi3False, 0.0, tStart, fmi3True, tEnd));
    CALL(FMI3EnterInitializationMode(supervisor, fmi3False, 0.0, tStart, fmi3True, tEnd));

    // Exchange data Controller -> Plantmodel
    CALL(FMI3GetFloat64(controller, controller_y_refs, 1, controller_vals, 1));
    CALL(FMI3SetFloat64(plant,      plantmodel_u_refs, 1, controller_vals, 1));

    // Exchange data Plantmodel -> Controller
    CALL(FMI3GetFloat64(plant,      plantmodel_y_refs, 1, plantmodel_vals, 1));
    CALL(FMI3SetFloat64(controller, controller_u_refs, 1, plantmodel_vals, 1));

    // Exchange data Plantmodel -> Supervisor
    CALL(FMI3SetFloat64(supervisor, supervisor_in_refs, 1, plantmodel_vals, 1));

    // Get clock r's interval
    CALL(FMI3GetIntervalDecimal(controller, controller_r_refs, 1, controller_interval_vals, controller_interval_qualifiers));
    controller_r_period = controller_interval_vals[0];
    controller_r_timer = controller_r_period;

    // Initialize event indicators
    CALL(FMI3GetEventIndicators(supervisor, supervisor_evt_vals, 1));
    supervisor_event_indicator = supervisor_evt_vals[0];

    CALL(FMI3ExitInitializationMode(controller));
    CALL(FMI3ExitInitializationMode(plant));
    CALL(FMI3ExitInitializationMode(supervisor));

    time = tStart;

    CALL(FMI3EnterContinuousTimeMode(controller));
    CALL(FMI3EnterContinuousTimeMode(plant));
    CALL(FMI3EnterContinuousTimeMode(supervisor));

    // Record initial outputs
    CALL(recordVariables(outputFile, controller, plant, time));

    while (time + h <= tEnd) {
        // Advance time and update timers
        time += h;
        controller_r_timer -= h;

        // Check for state events or time events.
        bool timeEvent = controller_r_timer <= 0.0;

        CALL(FMI3GetEventIndicators(supervisor, supervisor_evt_vals, 1));
        bool stateEvent = supervisor_event_indicator * supervisor_evt_vals[0] < 0.0;
        supervisor_event_indicator = supervisor_evt_vals[0];

        printf("Time event: %d \t State Event: %d \n", timeEvent, stateEvent);

        // Check if controller or supervisor need to execute
        if (timeEvent || stateEvent) {
            if (timeEvent && !stateEvent) {
                printf("Entering event mode for ticking clock r. \n");

                // Reset timer
                controller_r_timer = controller_r_period;

                // Put Controller into event mode, as clocks are about to tick
                CALL(FMI3EnterEventMode(controller));
                // Put Plantmodel into event mode, as its input is a discrete time variable
                CALL(FMI3EnterEventMode(plant));

                // Handle time event in controller
                CALL(handleTimeEventController(controller, plant));

                // Update discrete states of the controller
                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;
                fmi3Boolean nextEventTimeDefined = fmi3False;
                fmi3Boolean terminateSimulation = fmi3False;
                fmi3Boolean discreteStatesNeedUpdate = fmi3False;
                fmi3Float64 nextEventTime = INFINITY;
                CALL(FMI3UpdateDiscreteStates(controller, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));
                CALL(FMI3UpdateDiscreteStates(plant, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

                // Exit event mode
                CALL(FMI3EnterContinuousTimeMode(controller));
                CALL(FMI3EnterContinuousTimeMode(plant));
                printf("Exiting event mode. \n");
            }
            else if (!timeEvent && stateEvent) {
                printf("Entering event mode for ticking clock s. \n");

                // Put Supervisor and Controller into event mode, as clocks are about to tick. Note the flags used.
                CALL(FMI3EnterEventMode(supervisor));
                CALL(FMI3EnterEventMode(controller));

                // Handle state event supervisor
                CALL(handleStateEventSupervisor(controller, supervisor));

                // Update discrete states of the controller and supervisor
                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;
                fmi3Boolean nextEventTimeDefined = fmi3False;
                fmi3Boolean terminateSimulation = fmi3False;
                fmi3Boolean discreteStatesNeedUpdate = fmi3False;
                fmi3Float64 nextEventTime = INFINITY;
                CALL(FMI3UpdateDiscreteStates(supervisor, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));
                CALL(FMI3UpdateDiscreteStates(controller, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

                // Exit event mode
                CALL(FMI3EnterContinuousTimeMode(supervisor));
                CALL(FMI3EnterContinuousTimeMode(controller));
                printf("Exiting event mode. \n");
            }
            else {
                assert(timeEvent && stateEvent);

                // Reset timer
                controller_r_timer = controller_r_period;

                // Handle both time event and state event.
                printf("Entering event mode for ticking clocks s and r. \n");
                // Now we must respect the dependencies. The supervisor gets priority, and then the controller.

                // Put Supervisor and Controller into event mode, as clocks are about to tick.
                CALL(FMI3EnterEventMode(supervisor));
                CALL(FMI3EnterEventMode(controller));
                // Put Plantmodel into event mode, as its input is a discrete time variable
                CALL(FMI3EnterEventMode(plant));

                CALL(handleStateEventSupervisor(controller, supervisor));
                CALL(handleTimeEventController(controller, plant));

                // Update discrete states of the controller and supervisor
                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;
                fmi3Boolean nextEventTimeDefined = fmi3False;
                fmi3Boolean terminateSimulation = fmi3False;
                fmi3Boolean discreteStatesNeedUpdate = fmi3False;
                fmi3Float64 nextEventTime = INFINITY;
                CALL(FMI3UpdateDiscreteStates(supervisor, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));
                CALL(FMI3UpdateDiscreteStates(controller, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));
                CALL(FMI3UpdateDiscreteStates(plant, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

                // Exit event mode
                CALL(FMI3EnterContinuousTimeMode(supervisor));
                CALL(FMI3EnterContinuousTimeMode(controller));
                CALL(FMI3EnterContinuousTimeMode(plant));
                printf("Exiting event mode. \n");
            }
        }

        // Estimate next Plantmodel state
        CALL(FMI3GetContinuousStates(plant, plantmodel_vals, 1));
        CALL(FMI3GetContinuousStateDerivatives(plant, plantmodel_der_vals, 1));
        plantmodel_vals[0] += h * plantmodel_der_vals[0];

        // Set FMU time
        CALL(FMI3SetTime(plant, time));
        CALL(FMI3SetTime(controller, time));
        CALL(FMI3SetTime(supervisor, time));

        // Update Plantmodel state
        CALL(FMI3SetContinuousStates(plant, plantmodel_vals, 1));

        // Exchange data Plantmodel -> Supervisor
        CALL(FMI3GetFloat64(plant, plantmodel_y_refs, 1, plantmodel_vals, 1));
        CALL(FMI3SetFloat64(supervisor, supervisor_in_refs, 1, plantmodel_vals, 1));
        // Exchange data Plantmodel -> Controller
        CALL(FMI3SetFloat64(controller, controller_u_refs, 1, plantmodel_vals, 1));

        // Record data
        CALL(recordVariables(outputFile, controller, plant, time));
    }

    CALL(FMI3Terminate(controller));
    CALL(FMI3Terminate(plant));
    CALL(FMI3Terminate(supervisor));

TERMINATE:

    CALL(FMI3FreeInstance(controller));
    CALL(FMI3FreeInstance(plant));
    CALL(FMI3FreeInstance(supervisor));

    fclose(outputFile);

    puts("Done!");

    return status == FMIOK ? EXIT_SUCCESS : EXIT_FAILURE;
}
