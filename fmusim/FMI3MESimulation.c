#include <stdlib.h>
#include <math.h>

#include "FMIUtil.h"
#include "FMI3.h"
#include "FMI3MESimulation.h"


#define FMI_PATH_MAX 4096

#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus FMI3MESimulate(const FMISimulationSettings* s) {

    FMIStatus status = FMIOK;

    const bool needsCompletedIntegratorStep = s->modelDescription->modelExchange->needsCompletedIntegratorStep;

    FMIInstance* S = s->S;

    fmi3Float64 time;

    fmi3Boolean inputEvent = fmi3False;
    fmi3Boolean timeEvent  = fmi3False;
    fmi3Boolean stateEvent = fmi3False;
    fmi3Boolean stepEvent  = fmi3False;

    fmi3Boolean discreteStatesNeedUpdate          = fmi3True;
    fmi3Boolean terminateSimulation               = fmi3False;
    fmi3Boolean nominalsOfContinuousStatesChanged = fmi3False;
    fmi3Boolean valuesOfContinuousStatesChanged   = fmi3False;
    fmi3Boolean nextEventTimeDefined              = fmi3False;
    fmi3Float64 nextEventTime                     = INFINITY;

    size_t nSteps;
    fmi3Float64 nextRegularPoint;
    fmi3Float64 nextCommunicationPoint;
    fmi3Float64 nextInputEventTime;

    fmi3Boolean resetSolver;

    FMISolver* solver = NULL;

    char resourcePath[FMI_PATH_MAX] = "";

#ifdef _WIN32
    snprintf(resourcePath, FMI_PATH_MAX, "%s\\resources\\", s->unzipdir);
#else
    snprintf(resourcePath, FMI_PATH_MAX, "%s/resources/", s->unzipdir);
#endif

    CALL(FMI3InstantiateModelExchange(S,
        s->modelDescription->instantiationToken,
        resourcePath,
        s->visible,
        s->loggingOn
    ));

    time = s->startTime;

    if (s->initialFMUStateFile) {
        CALL(FMIRestoreFMUStateFromFile(S, s->initialFMUStateFile));
    }

    CALL(FMIApplyStartValues(S, s));

    if (!s->initialFMUStateFile) {

        // initialize
        CALL(FMI3EnterInitializationMode(S, s->tolerance > 0, s->tolerance, time, fmi3False, 0));
        
        CALL(FMIApplyInput(S, s->input, time,
            true,  // discrete
            true,  // continous
            false  // after event
        ));
        
        CALL(FMI3ExitInitializationMode(S));

        // initial event iteration
        do {

            CALL(FMI3UpdateDiscreteStates(S,
                &discreteStatesNeedUpdate,
                &terminateSimulation,
                &nominalsOfContinuousStatesChanged,
                &valuesOfContinuousStatesChanged,
                &nextEventTimeDefined,
                &nextEventTime));

            if (terminateSimulation) {
                goto TERMINATE;
            }

        } while (discreteStatesNeedUpdate);

        if (!nextEventTimeDefined) {
            nextEventTime = INFINITY;
        }

        CALL(FMI3EnterContinuousTimeMode(S));
    }

    CALL(FMIRecorderUpdateSizes(s->initialRecorder));
    CALL(FMIRecorderUpdateSizes(s->recorder));

    FMISolverParameters solverFunctions = {
        .modelInstance = S,
        .input = s->input,
        .startTime = time,
        .tolerance = s->tolerance,
        .setTime = (FMISolverSetTime)FMI3SetTime,
        .applyInput = (FMISolverApplyInput)FMIApplyInput,
        .getContinuousStates = (FMISolverGetContinuousStates)FMI3GetContinuousStates,
        .setContinuousStates = (FMISolverSetContinuousStates)FMI3SetContinuousStates,
        .getNominalsOfContinuousStates = (FMISolverGetNominalsOfContinuousStates)FMI3GetNominalsOfContinuousStates,
        .getContinuousStateDerivatives = (FMISolverGetContinuousStateDerivatives)FMI3GetContinuousStateDerivatives,
        .getEventIndicators = (FMISolverGetEventIndicators)FMI3GetEventIndicators,
        .logError = (FMISolverLogError)FMILogError
    };

    CALL(FMIGetNumberOfUnkownValues(S, s->modelDescription->nContinuousStates, s->modelDescription->derivatives, &solverFunctions.nx));
    CALL(FMIGetNumberOfUnkownValues(S, s->modelDescription->nEventIndicators, s->modelDescription->eventIndicators, &solverFunctions.nz));

    solver = s->solverCreate(&solverFunctions);
    
    if (!solver) {
        status = FMIError;
        goto TERMINATE;
    }

    nSteps = 0;

    CALL(FMISample(S, time, s->initialRecorder));

    for (;;) {

        CALL(FMISample(S, time, s->recorder));

        if (time >= s->stopTime) {
            break;
        }

        nextRegularPoint = s->startTime + (nSteps + 1) * s->outputInterval;

        nextCommunicationPoint = nextRegularPoint;

        nextInputEventTime = FMINextInputEvent(s->input, time);

        inputEvent = nextCommunicationPoint >= nextInputEventTime;

        timeEvent = nextCommunicationPoint >= nextEventTime;

        if (inputEvent || timeEvent) {
            nextCommunicationPoint = fmin(nextInputEventTime, nextEventTime);
        }

        CALL(s->solverStep(solver, nextCommunicationPoint, &time, &stateEvent));

        CALL(FMI3SetTime(S, time));

        CALL(FMIApplyInput(S, s->input, time,
            false, // discrete
            true,  // continous
            false  // after event
        ));

        if (time == nextRegularPoint) {
            nSteps++;
        }

        if (needsCompletedIntegratorStep) {

            CALL(FMI3CompletedIntegratorStep(S, fmi3True, &stepEvent, &terminateSimulation));

            if (terminateSimulation) {
                goto TERMINATE;
            }
        }

        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            CALL(FMISample(S, time, s->recorder));

            CALL(FMI3EnterEventMode(S));

            if (inputEvent) {
                CALL(FMIApplyInput(S, s->input, time,
                    true,  // discrete
                    true,  // continous
                    true   // after event
                ));
            }

            resetSolver = fmi3False;

            do {

                CALL(FMI3UpdateDiscreteStates(S, 
                    &discreteStatesNeedUpdate, 
                    &terminateSimulation, 
                    &nominalsOfContinuousStatesChanged, 
                    &valuesOfContinuousStatesChanged, 
                    &nextEventTimeDefined, 
                    &nextEventTime));
                
                if (terminateSimulation) {
                    CALL(FMISample(S, time, s->recorder));
                    goto TERMINATE;
                }

                resetSolver |= nominalsOfContinuousStatesChanged || valuesOfContinuousStatesChanged;

            } while (discreteStatesNeedUpdate);

            if (!nextEventTimeDefined) {
                nextEventTime = INFINITY;
            }

            CALL(FMI3EnterContinuousTimeMode(S));

            if (resetSolver) {
                s->solverReset(solver, time);
            }
        }

        if (s->stepFinished && !s->stepFinished(s, time)) {
            break;
        }
    }

    if (s->finalFMUStateFile) {
        CALL(FMISaveFMUStateToFile(S, s->finalFMUStateFile));
    }

TERMINATE:

    if (status < FMIError) {

        const FMIStatus terminateStatus = FMI3Terminate(S);

        if (terminateStatus > status) {
            status = terminateStatus;
        }
    }

    if (status != FMIFatal) {
        FMI3FreeInstance(S);
    }

    if (solver) {
        s->solverFree(solver);
    }

    return status;
}
