#include <stdlib.h>
#include <math.h>

#include "FMIUtil.h"
#include "FMI2.h"
#include "FMI2MESimulation.h"


#define FMI_PATH_MAX 4096

#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)

FMIStatus FMI2MESimulate(const FMISimulationSettings* s) {

    FMIStatus status = FMIOK;

    FMIInstance* S = s->S;

    const bool needsCompletedIntegratorStep = s->modelDescription->modelExchange->needsCompletedIntegratorStep;

    char resourceURI[FMI_PATH_MAX] = "";

#ifdef _WIN32
    snprintf(resourceURI, FMI_PATH_MAX, "%s\\resources\\", s->unzipdir);
#else
    snprintf(fmuResourceLocation, FMI_PATH_MAX, "%s/resources/", s->unzipdir);
#endif

    fmi2Real time;

    bool stateEvent = false;
    fmi2Boolean inputEvent = fmi2False;
    fmi2Boolean timeEvent  = fmi2False;
    fmi2Boolean stepEvent  = fmi2False;

    fmi2Boolean resetSolver;

    FMISolver* solver = NULL;

    size_t nSteps;
    fmi2Real nextRegularPoint;
    fmi2Real nextCommunicationPoint;
    fmi2Real nextInputEventTime;

    fmi2EventInfo eventInfo = { 
        .newDiscreteStatesNeeded           = fmi2False,
        .terminateSimulation               = fmi2False,
        .nominalsOfContinuousStatesChanged = fmi2False,
        .valuesOfContinuousStatesChanged   = fmi2False,
        .nextEventTimeDefined              = fmi2False,
        .nextEventTime                     = INFINITY
    };

    CALL(FMI2Instantiate(S,
        resourceURI,
        fmi2ModelExchange,
        s->modelDescription->instantiationToken,
        s->visible,
        s->loggingOn
    ));

    time = s->startTime;

    if (s->initialFMUStateFile) {
        CALL(FMIRestoreFMUStateFromFile(S, s->initialFMUStateFile));
    }

    CALL(FMIApplyStartValues(S, s));

    if (!s->initialFMUStateFile) {

        CALL(FMI2SetupExperiment(S, s->tolerance > 0, s->tolerance, time, fmi2False, 0));
        CALL(FMI2EnterInitializationMode(S));
        CALL(FMIApplyInput(S, s->input, time,
            true,  // discrete
            true,  // continous
            false  // after event
        ));
        CALL(FMI2ExitInitializationMode(S));

        do {

            CALL(FMI2NewDiscreteStates(S, &eventInfo));

            if (eventInfo.terminateSimulation) {
                goto TERMINATE;
            }

        } while (eventInfo.newDiscreteStatesNeeded);

        if (!eventInfo.nextEventTimeDefined) {
            eventInfo.nextEventTime = INFINITY;
        }

        CALL(FMI2EnterContinuousTimeMode(S));
    }

    const FMISolverParameters solverFunctions = {
        .modelInstance                 = S,
        .input                         = s->input,
        .startTime                     = time,
        .tolerance                     = s->tolerance,
        .nx                            = s->modelDescription->nContinuousStates,
        .nz                            = s->modelDescription->nEventIndicators,
        .setTime                       = (FMISolverSetTime)FMI2SetTime,
        .applyInput                    = (FMISolverApplyInput)FMIApplyInput,
        .getContinuousStates           = (FMISolverGetContinuousStates)FMI2GetContinuousStates,
        .setContinuousStates           = (FMISolverSetContinuousStates)FMI2SetContinuousStates,
        .getNominalsOfContinuousStates = (FMISolverGetNominalsOfContinuousStates)FMI2GetNominalsOfContinuousStates,
        .getContinuousStateDerivatives = (FMISolverGetContinuousStateDerivatives)FMI2GetDerivatives,
        .getEventIndicators            = (FMISolverGetEventIndicators)FMI2GetEventIndicators,
        .logError                      = (FMISolverLogError)FMILogError
    };

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

        timeEvent = nextCommunicationPoint >= eventInfo.nextEventTime;

        if (inputEvent || timeEvent) {
            nextCommunicationPoint = fmin(nextInputEventTime, eventInfo.nextEventTime);
        }

        CALL(s->solverStep(solver, nextCommunicationPoint, &time, &stateEvent));

        CALL(FMI2SetTime(S, time));

        CALL(FMIApplyInput(S, s->input, time,
            false,  // discrete
            true,   // continous
            false   // after event
        ));

        if (time == nextRegularPoint) {
            nSteps++;
        }

        if (needsCompletedIntegratorStep) {

            CALL(FMI2CompletedIntegratorStep(S, fmi2True, &stepEvent, &eventInfo.terminateSimulation));
            
            if (eventInfo.terminateSimulation) {
                goto TERMINATE;
            }
        }

        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            // record the values before the event
            CALL(FMISample(S, time, s->recorder));

            CALL(FMI2EnterEventMode(S));

            if (inputEvent) {
                CALL(FMIApplyInput(S, s->input, time,
                    true,  // discrete
                    true,  // continous
                    true   // after event
                ));
            }

            resetSolver = fmi2False;

            // event iteration
            do {
                CALL(FMI2NewDiscreteStates(S, &eventInfo));

                if (eventInfo.terminateSimulation) {
                    CALL(FMISample(S, time, s->recorder));
                    goto TERMINATE;
                }

                resetSolver |= eventInfo.nominalsOfContinuousStatesChanged || eventInfo.valuesOfContinuousStatesChanged;

            } while (eventInfo.newDiscreteStatesNeeded);

            if (!eventInfo.nextEventTimeDefined) {
                eventInfo.nextEventTime = INFINITY;
            }

            // enter Continuous-Time Mode
            CALL(FMI2EnterContinuousTimeMode(S));

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

        const FMIStatus terminateStatus = FMI2Terminate(S);

        if (terminateStatus > status) {
            status = terminateStatus;
        }
    }

    if (status != FMIFatal) {
        FMI2FreeInstance(S);
    }

    if (solver) {
        s->solverFree(solver);
    }

    return status;
}
