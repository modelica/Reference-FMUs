#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "fmi3Functions.h"
#include "config.h"
#include "util.h"

typedef struct {
    double* x;
    double* xd;
    double  Tf;
    int nStates;
    fmi3InstanceEnvironment env;
    fmi3CallbackIntermediateUpdate intermediateUpdate;
} ModelInstance;

#define GetStateVector(comp) (comp->x)
#define GetStateDervativeVector(comp)  (comp->xd)
#define GetNumberOfStates(comp)  (comp->nStates)
#define GetFinalTime(comp)  (comp->Tf)
#define GetInstanceEnvironment(comp)  (comp->env)

/* dummy fucntions*/
double *EvaluateDerivatives(ModelInstance *comp, double t1, double *x){return x;}
fmi3Boolean CheckEventIndicators(ModelInstance *comp, double t2, double *x) { return fmi3False; }
fmi3Boolean  if_STOP_button_is_pressed(fmi3InstanceEnvironment env) { return fmi3False;}
fmi3Boolean  if_an_unpredictable_event_has_happened(fmi3InstanceEnvironment env){return fmi3False;}
fmi3Status  Update_FMU_Variables(ModelInstance* comp) { return fmi3OK; }
fmi3Status  HandleStateEvent(ModelInstance* comp, fmi3Float64 t) { return fmi3OK; }
fmi3Boolean DoesThisFMUAllowEarlyReturn(ModelInstance* comp) { return fmi3OK; }
fmi3Boolean isValid(int nx, double *x){ return fmi3True; }
/* end of dummy fucntions*/

#ifndef fmi3DoStep
// tag::DoStepWithEarlyReturn[]
/* This is a sime implementation of fmi3DoStep inside the FMU supporting earlyReturn*/
fmi3Status fmi3DoStep(fmi3Instance instance,
    fmi3Float64 currentCommunicationPoint,
    fmi3Float64 communicationStepSize,
    fmi3Boolean noSetFMUStatePriorToCurrentPoint,
    fmi3Boolean* terminate,
    fmi3Boolean* earlyReturn,
    fmi3Float64* lastSuccessfulTime) {

    ModelInstance* comp = (ModelInstance*)instance;
    fmi3InstanceEnvironment  env = GetInstanceEnvironment(comp);
    double* x = GetStateVector(comp);
    double* xd = GetStateDervativeVector(comp);
    int  nStates = GetNumberOfStates(comp);
    double Tf = GetFinalTime(comp);// Tf is a Stop time beyond which the FMU cannot go (it may be INF)

    const int N = 10;
    fmi3Float64  time = currentCommunicationPoint;
    fmi3Float64  h = communicationStepSize / N;

    fmi3Boolean earlyReturnRequested, StateEventHappened;
    fmi3Float64 earlyReturnTime;
    fmi3Boolean AllowEarlyReturn;
    fmi3Float64 t1, t2;
    int i, n;

    *earlyReturn = fmi3False;
    *lastSuccessfulTime = currentCommunicationPoint;
    *terminate = fmi3False;

    for (i = 0; i < N; i++) {
        t1 = time + (i)*h;
        t2 = t1 + h;

        if (t2 > Tf) {// the FMU cannot simulate beyond Tf, so it requests a terminate
            t2 = Tf;
            h = t2 - t1;
        }

        xd = EvaluateDerivatives(comp, t1, x);
        if (!isValid(nStates, xd)) { //if xd contains INF or NAN, doStep should return with fmi3Discard
            *earlyReturn = fmi3False; // *earlyReturn will be ignored by the cosim algorithm
            *lastSuccessfulTime = t1;
            *terminate = fmi3False;
            return fmi3Discard;
        }

        for (n = 0; n < nStates;n++) {
            x[n] += xd[n] * h; // Explicit Euler method
        }
        Update_FMU_Variables(comp);//e.g., update outputs

        // checks if an event-indicator has chnaged the sign wrt the previous values at time=t1
        StateEventHappened = CheckEventIndicators(comp, t2, x); 

        earlyReturnRequested = fmi3False;
        earlyReturnTime = t2;

        // Upon the internal implementation of the FMU, FMU can allow or disallow early-return
        AllowEarlyReturn = DoesThisFMUAllowEarlyReturn(comp);

        comp->intermediateUpdate(
            env,                   // instanceEnvironment
            t2,                    // intermediateUpdateTime
            StateEventHappened,    // eventOccurred
            fmi3False,             // clocksTicked
            fmi3True,              // intermediateVariableSetAllowed
            fmi3True,              // intermediateVariableGetAllowed
            fmi3True,              // intermediateStepFinished
            AllowEarlyReturn,      // canReturnEarly
            &earlyReturnRequested, // *earlyReturnRequested
            &earlyReturnTime);     // *earlyReturnTime

        if (AllowEarlyReturn) {
            if (earlyReturnRequested && t2 >= earlyReturnTime) {
                *earlyReturn = fmi3True;  // will do early return only if an event has been detected and the cosim algorithm allows the early return
                *lastSuccessfulTime = t2;
                *terminate = fmi3False;
                return fmi3OK;
            }
            else {
                if (StateEventHappened) {
                    // Since the cosim algorithm did not request the early return, the detected state-event needs to be handled internally
                    HandleStateEvent(comp, t2);
                }
            }
        }
        else {
            if (StateEventHappened) {
                // Since earlyRetrun is not allowed by the FMU, the detected state-event needs be handled internally
                HandleStateEvent(comp, t2);
            }
        }

        if (t2 >= Tf) {// the FMU cannot simulate beyond Tf, so it requests a terminate
            *earlyReturn = fmi3False;// *earlyReturn will be ignored by the cosim algorithm
            *lastSuccessfulTime = t2;
            *terminate = fmi3True;
            return fmi3OK;
        }
    }

    *lastSuccessfulTime = t2;
    return fmi3OK;
}

// end::DoStepWithEarlyReturn[]

#endif

////////// Cosimulation algorithm ///////////////////////////////////////////////

typedef struct {
    fmi3Instance instance;
    fmi3Float64 intermediateUpdateTime;
} InstanceEnvironment;


void cb_intermediateUpdate(fmi3InstanceEnvironment instanceEnvironment,
    fmi3Float64 intermediateUpdateTime,
    fmi3Boolean eventOccurred,
    fmi3Boolean clocksTicked,
    fmi3Boolean intermediateVariableSetAllowed,
    fmi3Boolean intermediateVariableGetAllowed,
    fmi3Boolean intermediateStepFinished,
    fmi3Boolean canReturnEarly,
    fmi3Boolean* earlyReturnRequested,
    fmi3Float64* earlyReturnTime) {

    if (!instanceEnvironment) {return;}
    if (!intermediateStepFinished) {return;}

    InstanceEnvironment* env = (InstanceEnvironment*)instanceEnvironment;
    fmi3Boolean  userStopRequest = if_STOP_button_is_pressed(env); //Check asynchronously e.g. through a thread. 
    fmi3Boolean  EventHappened = if_an_unpredictable_event_has_happened(env);//e.g. an event in another FMU

    // remember the intermediateUpdateTime
    env->intermediateUpdateTime = intermediateUpdateTime;

    if (canReturnEarly) {
        *earlyReturnRequested = eventOccurred || clocksTicked || userStopRequest|| EventHappened;
        *earlyReturnTime = intermediateUpdateTime;
    }
}

int main(int argc, char* argv[]) {

    fmi3Boolean terminate, earlyReturn;
    fmi3Float64 lastSuccessfulTime;
    const fmi3Float64 startTime = 0;
    const fmi3Float64 stopTime = 3;
    const fmi3Float64 h = 0.01;
    fmi3Boolean newDiscreteStatesNeeded = fmi3False;
    fmi3Boolean terminateSimulation = fmi3False;
    fmi3Boolean nominalsOfContinuousStatesChanged = fmi3False;
    fmi3Boolean valuesOfContinuousStatesChanged = fmi3False;
    fmi3Boolean nextEventTimeDefined = fmi3False;
    fmi3Float64 nextEventTime=0.0;

    InstanceEnvironment instanceEnvironment = {
        .instance = NULL,
        .intermediateUpdateTime = startTime,
    };

    // Instantiate the FMU 
    fmi3Instance s = fmi3InstantiateCoSimulation(
        "Fmu1",               // instanceName
        INSTANTIATION_TOKEN,    // instantiationToken
        NULL,                   // resourceLocation
        fmi3False,              // visible
        fmi3False,              // loggingOn
        fmi3False,              // eventModeRequired
        NULL,                   // requiredIntermediateVariables
        0,                      // nRequiredIntermediateVariables
        &instanceEnvironment,   // instanceEnvironment
        cb_logMessage,          // logMessage
        cb_intermediateUpdate); // intermediateUpdate

    if (s == NULL) {
        puts("Failed to instantiate FMU.");
        return EXIT_FAILURE;
    }

    instanceEnvironment.instance = s;

    // Set all start values
    // fmi3Set{VariableType}()

    fmi3Status status = fmi3OK;

    // Initialize the slave
    CHECK_STATUS(fmi3EnterInitializationMode(s, fmi3False, 0.0, startTime, fmi3True, stopTime));
    // Set the input values at time = startTime
    // fmi3Set{VariableType}()
    CHECK_STATUS(fmi3ExitInitializationMode(s));

    fmi3Float64 tc = startTime; // Starting master time
    fmi3Float64 step = h;       // Starting non-zero step size

    while (tc < stopTime) {

        //fmi3Set{VariableType}() //setInputs
        CHECK_STATUS(fmi3DoStep(s, tc, step, fmi3False, &terminate, &earlyReturn, &lastSuccessfulTime));
        if (terminate) break;

        if (earlyReturn) {
            fmi3EnterEventMode(s,  // fmi3Instance 
                 fmi3False,        // stepEvent
                 NULL,             // rootsFound[],
                 0,                // nEventIndicators,
                fmi3False);        // timeEvent

            newDiscreteStatesNeeded=fmi3True;
            while (newDiscreteStatesNeeded == fmi3True) {
                fmi3NewDiscreteStates(
                    s,                                  // instance,
                    &newDiscreteStatesNeeded,           // * newDiscreteStatesNeeded,
                    &terminateSimulation,               // * terminateSimulation,
                    &nominalsOfContinuousStatesChanged, // * nominalsOfContinuousStatesChanged,
                    &valuesOfContinuousStatesChanged,   // * valuesOfContinuousStatesChanged,
                    &nextEventTimeDefined,              // * nextEventTimeDefined,
                    &nextEventTime);                    // * nextEventTime
                //fmi3Get{ VariableType }() //get outputs
                if (terminateSimulation) break;
            }
            if (terminateSimulation) break;
            fmi3EnterStepMode(s);
            tc = lastSuccessfulTime;
            step = h - fmod(tc, h); // finish the step
        }else{
            tc += step;
            step = h;
        }
        //fmi3Get{ VariableType }() //get outputs
    };

    fmi3Status terminateStatus;

TERMINATE:

    if (s && status != fmi3Error && status != fmi3Fatal) {
        terminateStatus = fmi3Terminate(s);
    }

    if (s && status != fmi3Fatal && terminateStatus != fmi3Fatal) {
        fmi3FreeInstance(s);
    }

    return status == fmi3OK ? EXIT_SUCCESS : EXIT_FAILURE;
}