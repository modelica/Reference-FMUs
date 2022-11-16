#include <stdlib.h>
#include <math.h>
#include "fmusim_fmi2.h"
#include "fmusim_fmi2_me.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

typedef struct {
    FMIInstance* S;
    double time;
    size_t nx;
    double* x;
    double* dx;
    size_t nz;
    double* z;
    double* prez;
} Solver;

static void* createSolver(FMIInstance* S, const FMIModelDescription * modelDescription, const FMUStaticInput* input, double startTime) {

    Solver* solver = (Solver*)calloc(1, sizeof(Solver));

    if (!solver) {
        return NULL;
    }

    solver->S = S;
    solver->time = startTime;

    solver->nx = modelDescription->nContinuousStates;
    solver->x  = (double*)calloc(solver->nx, sizeof(double));
    solver->dx = (double*)calloc(solver->nx, sizeof(double));

    solver->nz   = modelDescription->nEventIndicators;
    solver->z    = (double*)calloc(solver->nx, sizeof(double));
    solver->prez = (double*)calloc(solver->nx, sizeof(double));

    // initialize the event indicators
    FMI2GetEventIndicators(solver->S, solver->prez, solver->nz);

    return solver;
}

static void solverStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent) {

    FMI2GetContinuousStates(solver->S, solver->x, solver->nx);
    
    FMI2GetDerivatives(solver->S, solver->dx, solver->nx);

    const double dt = nextTime - solver->time;

    // do one step
    for (size_t i = 0; i < solver->nx; i++) {
        solver->x[i] += dt * solver->dx[i];
    }

    FMI2SetContinuousStates(solver->S, solver->x, solver->nx);

    FMI2GetEventIndicators(solver->S, solver->z, solver->nz);

    *stateEvent = false;

    for (size_t i = 0; i < solver->nz; i++) {

        if (solver->prez[i] <= 0 && solver->z[i] > 0) {
            *stateEvent = true;  // -\+
        } else if (solver->prez[i] > 0 && solver->z[i] <= 0) {
            *stateEvent = true;  // +/-
        }

        solver->prez[i] = solver->z[i];
    }

    solver->time = nextTime;
    *timeReached = nextTime;
}

static void solverReset(Solver* solver, double time) {
    FMI2GetEventIndicators(solver->S, solver->prez, solver->nz);
}

static void freeSolver(Solver* solver) {
    // TODO
}
 

FMIStatus simulateFMI2ME(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourceURI,
    FMISimulationResult* result,
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[],
    double startTime,
    double stepSize,
    double stopTime,
    const FMUStaticInput * input) {

    //const size_t nx = modelDescription->nContinuousStates;
    //const size_t nz = modelDescription->nEventIndicators;
    
    const fmi2Real fixedStep = stepSize;

    fmi2Boolean inputEvent = fmi2False;
    fmi2Boolean timeEvent  = fmi2False;
    fmi2Boolean stateEvent = fmi2False;
    fmi2Boolean stepEvent  = fmi2False;
    fmi2Boolean nominalsChanged = fmi2False;
    fmi2Boolean statesChanged = fmi2False;

    Solver* solver = NULL;

    FMIStatus status = FMIOK;

    CALL(FMI2Instantiate(S,
        resourceURI,                          // fmuResourceLocation
        fmi2ModelExchange,                    // fmuType
        modelDescription->instantiationToken, // fmuGUID
        fmi2False,                            // visible
        fmi2False                             // loggingOn
    ));

    fmi2Real time = startTime;

    // set start values
    CALL(applyStartValuesFMI2(S, nStartValues, startVariables, startValues));
    CALL(FMIApplyInput(S, input, time,
        true,  // discrete
        true,  // continous
        false  // after event
    ));

    // initialize
    CALL(FMI2SetupExperiment(S, fmi2False, 0.0, time, fmi2True, stopTime));
    CALL(FMI2EnterInitializationMode(S));
    CALL(FMI2ExitInitializationMode(S));

    fmi2EventInfo eventInfo = { 0 };

    // intial event iteration
    do {

        CALL(FMI2NewDiscreteStates(S, &eventInfo));

        if (eventInfo.terminateSimulation) {
            goto TERMINATE;
        }
    } while (eventInfo.newDiscreteStatesNeeded);

    CALL(FMI2EnterContinuousTimeMode(S));

    solver = createSolver(S, modelDescription, input, time);

    CALL(FMISample(S, time, result));

    while (time < stopTime) {
    
        fmi2Real nextTime = time + stepSize;

        const double nextInputEventTime = FMINextInputEvent(input, time);

        inputEvent = time >= nextInputEventTime;
        
        solverStep(solver, nextTime, &time, &stateEvent);

        CALL(FMI2SetTime(S, time));

        // apply continuous inputs
        CALL(FMIApplyInput(S, input, time,
            false,  // discrete
            true,   // continous
            false   // after event
        ));

        // check for step event, e.g.dynamic state selection
        CALL(FMI2CompletedIntegratorStep(S, fmi2True, &stepEvent, &eventInfo.terminateSimulation));

        if (eventInfo.terminateSimulation) {
            goto TERMINATE;
        }

        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            // record the values before the event
            CALL(FMISample(S, time, result));

            CALL(FMI2EnterEventMode(S));

            if (inputEvent) {
                CALL(FMIApplyInput(S, input, time,
                    true,  // discrete
                    true,  // continous
                    true   // after event
                ));
            }

            nominalsChanged = fmi2False;
            statesChanged = fmi2False;

            // event iteration
            do {
                // set inputs at super dense time point

                // update discrete states
                CALL(FMI2NewDiscreteStates(S, &eventInfo));

                // get output at super dense time point

                nominalsChanged |= eventInfo.nominalsOfContinuousStatesChanged;
                statesChanged   |= eventInfo.valuesOfContinuousStatesChanged;

                if (eventInfo.terminateSimulation) {
                    goto TERMINATE;
                }

            } while (eventInfo.newDiscreteStatesNeeded);

            // enter Continuous-Time Mode
            CALL(FMI2EnterContinuousTimeMode(S));

            solverReset(solver, time);

            // record outputs after the event
            CALL(FMISample(S, time, result));
        }

        // record outputs
        CALL(FMISample(S, time, result));
    }


    //unsigned long step = 0;

    //while (!eventInfo.terminateSimulation) {

    //    // detect input and time events
    //    const double nextEventTime = FMINextInputEvent(input, time);
    //    
    //    inputEvent = time >= nextEventTime;

    //    timeEvent = eventInfo.nextEventTimeDefined && time >= eventInfo.nextEventTime;

    //    const bool eventOccurred = inputEvent || timeEvent || stateEvent || stepEvent;

    //    // handle events
    //    if (eventOccurred) {

    //        CALL(FMI2EnterEventMode(S));

    //        if (inputEvent) {
    //            CALL(FMIApplyInput(S, input, time,
    //                true,  // discrete
    //                true,  // continous
    //                true   // after event
    //            ));
    //        }

    //        fmi2Boolean nominalsChanged = fmi2False;
    //        fmi2Boolean statesChanged   = fmi2False;

    //        // event iteration
    //        do {
    //            // set inputs at super dense time point

    //            // update discrete states
    //            CALL(FMI2NewDiscreteStates(S, &eventInfo));

    //            // get output at super dense time point

    //            nominalsChanged |= eventInfo.nominalsOfContinuousStatesChanged;
    //            statesChanged   |= eventInfo.valuesOfContinuousStatesChanged;

    //            if (eventInfo.terminateSimulation) {
    //                goto TERMINATE;
    //            }

    //        } while (eventInfo.newDiscreteStatesNeeded);

    //        // enter Continuous-Time Mode
    //        CALL(FMI2EnterContinuousTimeMode(S));

    //        // retrieve solution at simulation (re)start
    //        CALL(FMISample(S, time, result));

    //        if (nx > 0) {
    //            if (statesChanged) {
    //                // the model signals a value change of states, retrieve them
    //                CALL(FMI2GetContinuousStates(S, x, nx));
    //            }

    //            if (nominalsChanged) {
    //                // the meaning of states has changed; retrieve new nominal values
    //                CALL(FMI2GetNominalsOfContinuousStates(S, x_nominal, nx));
    //            }
    //        }

    //        // record outputs after the event
    //        CALL(FMISample(S, time, result));
    //    }

    //    if (time >= stopTime) {
    //        goto TERMINATE;
    //    }

    //    if (nx > 0) {
    //        // compute continuous state derivatives
    //        CALL(FMI2GetDerivatives(S, der_x, nx));
    //    }

    //    // advance time
    //    time = startTime + (++step) * fixedStep;

    //    CALL(FMI2SetTime(S, time));

    //    // apply continuous inputs
    //    CALL(FMIApplyInput(S, input, time,
    //        false,  // discrete
    //        true,   // continous
    //        false   // after event
    //    ));

    //    if (nx > 0) {
    //        // set states at t = time and perform one step
    //        for (size_t i = 0; i < nx; i++) {
    //            x[i] += fixedStep * der_x[i]; // forward Euler method
    //        }

    //        CALL(FMI2SetContinuousStates(S, x, nx));
    //    }

    //    if (nz > 0) {
    //        stateEvent = fmi2False;

    //        if (eventOccurred) {
    //            // reset the previous event indicators
    //            CALL(FMI2GetEventIndicators(S, previous_z, nz));
    //        } else {
    //            // get event indicators at t = time
    //            CALL(FMI2GetEventIndicators(S, z, nz));

    //            for (size_t i = 0; i < nz; i++) {

    //                // check for zero crossings
    //                if (previous_z[i] <= 0 && z[i] > 0) {
    //                    rootsFound[i] = 1;   // -\+
    //                } else  if (previous_z[i] > 0 && z[i] <= 0) {
    //                    rootsFound[i] = -1;  // +/-
    //                } else {
    //                    rootsFound[i] = 0;   // no zero crossing
    //                }

    //                stateEvent |= rootsFound[i];

    //                previous_z[i] = z[i]; // remember the current value
    //            }
    //        }
    //    }

    //    // inform the model about an accepted step
    //    CALL(FMI2CompletedIntegratorStep(S, fmi2True, &stepEvent, &eventInfo.terminateSimulation));

    //    if (eventInfo.terminateSimulation) {
    //        goto TERMINATE;
    //    }

    //    // get continuous output
    //    CALL(FMISample(S, time, result));
    //}

TERMINATE:

    if (solver) {
        freeSolver(solver);
    }

    return status;
}