#include <stdlib.h>
#include <math.h>

#include "FMIUtil.h"
#include "FMI3.h"
#include "FMI3MESimulation.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus FMI3MESimulate(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourcePath,
    FMIRecorder* recorder,
    const FMIStaticInput * input,
    const FMISimulationSettings * settings) {

    const bool needsCompletedIntegratorStep = modelDescription->modelExchange->needsCompletedIntegratorStep;

    FMIStatus status = FMIOK;

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

    CALL(FMI3InstantiateModelExchange(S,
        modelDescription->instantiationToken,
        resourcePath,
        settings->visible,
        settings->loggingOn
    ));

    time = settings->startTime;

    if (settings->initialFMUStateFile) {
        CALL(FMIRestoreFMUStateFromFile(S, settings->initialFMUStateFile));
    }

    CALL(FMIApplyStartValues(S, settings));

    if (!settings->initialFMUStateFile) {

        // initialize
        CALL(FMI3EnterInitializationMode(S, settings->tolerance > 0, settings->tolerance, time, fmi3False, 0));
        
        CALL(FMIApplyInput(S, input, time,
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

    CALL(FMIRecorderUpdateSizes(recorder));

    FMISolverParameters solverFunctions = {
        .modelInstance = S,
        .input = input,
        .startTime = time,
        .tolerance = settings->tolerance,
        .setTime = (FMISolverSetTime)FMI3SetTime,
        .applyInput = (FMISolverApplyInput)FMIApplyInput,
        .getContinuousStates = (FMISolverGetContinuousStates)FMI3GetContinuousStates,
        .setContinuousStates = (FMISolverSetContinuousStates)FMI3SetContinuousStates,
        .getNominalsOfContinuousStates = (FMISolverGetNominalsOfContinuousStates)FMI3GetNominalsOfContinuousStates,
        .getContinuousStateDerivatives = (FMISolverGetContinuousStateDerivatives)FMI3GetContinuousStateDerivatives,
        .getEventIndicators = (FMISolverGetEventIndicators)FMI3GetEventIndicators,
        .logError = (FMISolverLogError)FMILogError
    };

    CALL(FMIGetNumberOfUnkownValues(S, modelDescription->nContinuousStates, modelDescription->derivatives, &solverFunctions.nx));
    CALL(FMIGetNumberOfUnkownValues(S, modelDescription->nEventIndicators, modelDescription->eventIndicators, &solverFunctions.nz));

    solver = settings->solverCreate(&solverFunctions);
    
    if (!solver) {
        status = FMIError;
        goto TERMINATE;
    }

    nSteps = 0;

    for (;;) {

        CALL(FMISample(S, time, recorder));

        if (time >= settings->stopTime) {
            break;
        }

        nextRegularPoint = settings->startTime + (nSteps + 1) * settings->outputInterval;

        nextCommunicationPoint = nextRegularPoint;

        nextInputEventTime = FMINextInputEvent(input, time);

        inputEvent = nextCommunicationPoint >= nextInputEventTime;

        timeEvent = nextCommunicationPoint >= nextEventTime;

        if (inputEvent || timeEvent) {
            nextCommunicationPoint = fmin(nextInputEventTime, nextEventTime);
        }

        CALL(settings->solverStep(solver, nextCommunicationPoint, &time, &stateEvent));

        CALL(FMI3SetTime(S, time));

        CALL(FMIApplyInput(S, input, time,
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

            CALL(FMISample(S, time, recorder));

            CALL(FMI3EnterEventMode(S));

            if (inputEvent) {
                CALL(FMIApplyInput(S, input, time,
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
                    CALL(FMISample(S, time, recorder));
                    goto TERMINATE;
                }

                resetSolver |= nominalsOfContinuousStatesChanged || valuesOfContinuousStatesChanged;

            } while (discreteStatesNeedUpdate);

            if (!nextEventTimeDefined) {
                nextEventTime = INFINITY;
            }

            CALL(FMI3EnterContinuousTimeMode(S));

            if (resetSolver) {
                settings->solverReset(solver, time);
            }
        }

        if (settings->stepFinished && !settings->stepFinished(settings, time)) {
            break;
        }
    }

    if (settings->finalFMUStateFile) {
        CALL(FMISaveFMUStateToFile(S, settings->finalFMUStateFile));
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
        settings->solverFree(solver);
    }

    return status;
}
