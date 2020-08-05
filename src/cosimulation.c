/**************************************************************
 *  Copyright (c) Modelica Association Project "FMI".         *
 *  All rights reserved.                                      *
 *  This file is part of the Reference FMUs. See LICENSE.txt  *
 *  in the project root for license information.              *
 **************************************************************/

#include <stdlib.h>  // for calloc(), free()
#include <float.h>   // for DBL_EPSILON
#include <math.h>    // for fabs()
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "cosimulation.h"

#if FMI_VERSION == 3
#include "fmi3Functions.h"
#endif


ModelInstance *createModelInstance(
    loggerType cbLogger,
    intermediateUpdateType intermediateUpdate,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn,
    InterfaceType interfaceType,
    bool returnEarly) {

    ModelInstance *comp = NULL;

    if (!cbLogger) {
        return NULL;
    }

    if (!instanceName || strlen(instanceName) == 0) {
        cbLogger(componentEnvironment, "?", Error, "error", "Missing instance name.");
        return NULL;
    }

    if (!instantiationToken || strlen(instantiationToken) == 0) {
        cbLogger(componentEnvironment, instanceName, Error, "error", "Missing GUID.");
        return NULL;
    }

    if (strcmp(instantiationToken, INSTANTIATION_TOKEN)) {
        cbLogger(componentEnvironment, instanceName, Error, "error", "Wrong GUID.");
        return NULL;
    }

    comp = (ModelInstance *)calloc(1, sizeof(ModelInstance));

    if (comp) {

        // set the callbacks
        comp->componentEnvironment = componentEnvironment;
        comp->logger = cbLogger;
        comp->intermediateUpdate = intermediateUpdate;
        comp->lockPreemtion = NULL;
        comp->unlockPreemtion = NULL;

        comp->instanceName = (char *)calloc(1 + strlen(instanceName), sizeof(char));

        // resourceLocation is NULL for FMI 1.0 ME
        if (resourceLocation) {
            comp->resourceLocation = (char *)calloc(1 + strlen(resourceLocation), sizeof(char));
            strcpy((char *)comp->resourceLocation, (char *)resourceLocation);
        } else {
            comp->resourceLocation = NULL;
        }

        comp->status = OK;

        comp->modelData = (ModelData *)calloc(1, sizeof(ModelData));

        comp->logEvents = loggingOn;
        comp->logErrors = true; // always log errors

        comp->nSteps = 0;

        comp->returnEarly = false;
    }

    if (!comp || !comp->modelData || !comp->instanceName) {
        logError(comp, "Out of memory.");
        return NULL;
    }

    comp->time = 0; // overwrite in fmi*SetupExperiment, fmi*SetTime
    strcpy((char *)comp->instanceName, (char *)instanceName);
    comp->type = interfaceType;

    comp->state = Instantiated;
    comp->isNewEventIteration = false;

    comp->newDiscreteStatesNeeded = false;
    comp->terminateSimulation = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->valuesOfContinuousStatesChanged = false;
    comp->nextEventTimeDefined = false;
    comp->nextEventTime = 0;

    setStartValues(comp); // to be implemented by the includer of this file
    comp->isDirtyValues = true; // because we just called setStartValues

#if NZ > 0
    comp->z    = calloc(sizeof(double), NZ);
    comp->prez = calloc(sizeof(double), NZ);
#else
    comp->z    = NULL;
    comp->prez = NULL;
#endif

    return comp;
}

void freeModelInstance(ModelInstance *comp) {
    free((void *)comp->instanceName);
    free(comp->z);
    free(comp->prez);
    free(comp);
}

bool invalidNumber(ModelInstance *comp, const char *f, const char *arg, size_t actual, size_t expected) {

    if (actual != expected) {
        comp->state = modelError;
        logError(comp, "%s: Invalid argument %s = %d. Expected %d.", f, arg, actual, expected);
        return true;
    }

    return false;
}

bool invalidState(ModelInstance *comp, const char *f, int statesExpected) {

    if (!comp) {
        return true;
    }

    // TODO: add missing states and check state
    return false;

//    if (!(comp->state & statesExpected)) {
//        comp->state = modelError;
//        logError(comp, "%s: Illegal call sequence.", f);
//        return true;
//    }
//
//    return false;
}

bool nullPointer(ModelInstance* comp, const char *f, const char *arg, const void *p) {

    if (!p) {
        comp->state = modelError;
        logError(comp, "%s: Invalid argument %s = NULL.", f, arg);
        return true;
    }

    return false;
}

Status setDebugLogging(ModelInstance *comp, bool loggingOn, size_t nCategories, const char * const categories[]) {

    if (loggingOn) {
        for (size_t i = 0; i < nCategories; i++) {
            if (categories[i] == NULL) {
                logError(comp, "Log category[%d] must not be NULL", i);
                return Error;
            } else if (strcmp(categories[i], "logEvents") == 0) {
                comp->logEvents = true;
            } else if (strcmp(categories[i], "logStatusError") == 0) {
                comp->logErrors = true;
            } else {
                logError(comp, "Log category[%d] must be one of logEvents or logStatusError but was %s", i, categories[i]);
                return Error;
            }
        }
    } else {
        // disable logging
        comp->logEvents = false;
        comp->logErrors = false;
    }

    return OK;
}

static void logMessage(ModelInstance *comp, int status, const char *category, const char *message, va_list args) {

    va_list args1;
    size_t len = 0;
    char *buf = "";

    va_copy(args1, args);
    len = vsnprintf(buf, len, message, args1);
    va_end(args1);

    va_copy(args1, args);
    buf = (char *)calloc(len + 1, sizeof(char));
    vsnprintf(buf, len + 1, message, args);
    va_end(args1);

    // no need to distinguish between FMI versions since we're not using variadic arguments
    comp->logger(comp->componentEnvironment, comp->instanceName, status, category, buf);

    free(buf);
}

void logEvent(ModelInstance *comp, const char *message, ...) {

    if (!comp || !comp->logEvents) return;

    va_list args;
    va_start(args, message);
    logMessage(comp, OK, "logEvents", message, args);
    va_end(args);
}

void logError(ModelInstance *comp, const char *message, ...) {

    if (!comp || !comp->logErrors) return;

    va_list args;
    va_start(args, message);
    logMessage(comp, Error, "logStatusError", message, args);
    va_end(args);
}

// default implementations
#if NZ < 1
void getEventIndicators(ModelInstance *comp, double z[], size_t nz) {
    UNUSED(comp)
    UNUSED(z)
    UNUSED(nz)
    // do nothing
}
#endif

#ifndef GET_FLOAT64
Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef GET_UINT16
Status getUInt16(ModelInstance* comp, ValueReference vr, uint16_t *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef GET_INT32
Status getInt32(ModelInstance* comp, ValueReference vr, int *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef GET_BOOLEAN
Status getBoolean(ModelInstance* comp, ValueReference vr, bool *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef GET_STRING
Status getString(ModelInstance* comp, ValueReference vr, const char **value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef GET_BINARY
Status getBinary(ModelInstance* comp, ValueReference vr, size_t size[], const char *value[], size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(size)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_FLOAT64
Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_UINT16
Status setUInt16(ModelInstance* comp, ValueReference vr, const uint16_t *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_INT32
Status setInt32(ModelInstance* comp, ValueReference vr, const int *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_BOOLEAN
Status setBoolean(ModelInstance* comp, ValueReference vr, const bool *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_STRING
Status setString(ModelInstance* comp, ValueReference vr, const char *const *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_BINARY
Status setBinary(ModelInstance* comp, ValueReference vr, const size_t size[], const char *const value[], size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(size)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef ACTIVATE_CLOCK
Status activateClock(ModelInstance* comp, ValueReference vr) {
    UNUSED(comp)
    UNUSED(vr)
    return Error;
}
#endif

#ifndef GET_CLOCK
Status getClock(ModelInstance* comp, ValueReference vr, int* value) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    return Error;
}
#endif

#ifndef ACTIVATE_MODEL_PARTITION
Status activateModelPartition(ModelInstance* comp, ValueReference vr, double activationTime) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(activationTime)
    return Error;
}
#endif

#if NX < 1
void getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    UNUSED(comp)
    UNUSED(x)
    UNUSED(nx)
}

void setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    UNUSED(comp)
    UNUSED(x)
    UNUSED(nx)
}

void getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    UNUSED(comp)
    UNUSED(dx)
    UNUSED(nx)
}
#endif

#ifndef GET_PARTIAL_DERIVATIVE
Status getPartialDerivative(ModelInstance *comp, ValueReference unknown, ValueReference known, double *partialDerivative) {
    UNUSED(comp)
    UNUSED(unknown)
    UNUSED(known)
    UNUSED(partialDerivative)
    return Error;
}
#endif

Status doStep(ModelInstance *comp, double t, double tNext, int* earlyReturn, double* lastSuccessfulTime) {

    UNUSED(t)  // TODO: check t == comp->time ?

    bool stateEvent, timeEvent;
    Status status = OK;

#if NZ > 0
    double *temp = NULL;
#endif

#if NX > 0
    double  x[NX] = { 0 };
    double dx[NX] = { 0 };
#endif

    double epsilon = (1.0 + fabs(comp->time)) * DBL_EPSILON;

    while (comp->time + FIXED_SOLVER_STEP < tNext + epsilon) {

#if NX > 0
        getContinuousStates(comp, x, NX);
        getDerivatives(comp, dx, NX);

        // forward Euler step
        for (int i = 0; i < NX; i++) {
            x[i] += FIXED_SOLVER_STEP * dx[i];
        }

        setContinuousStates(comp, x, NX);
#endif

        stateEvent = false;

#if NZ > 0
        getEventIndicators(comp, comp->z, NZ);

        // check for zero-crossings
        for (int i = 0; i < NZ; i++) {
            stateEvent |= comp->prez[i] < 0 && comp->z[i] >= 0;
            stateEvent |= comp->prez[i] > 0 && comp->z[i] <= 0;
        }

        // remember the current event indicators
        temp = comp->z;
        comp->z = comp->prez;
        comp->prez = temp;
#endif

        // check for time event
        timeEvent = comp->nextEventTimeDefined && (comp->time + FIXED_SOLVER_STEP * 1e-2) >= comp->nextEventTime;

        // log events
        if (timeEvent) logEvent(comp, "Time event detected at t=%g s.", comp->time);
        if (stateEvent) logEvent(comp, "State event detected at t=%g s.", comp->time);

        if (stateEvent || timeEvent) {

            eventUpdate(comp);

            comp->returnEarly = comp->nextEventTime < t + tNext;

#if NZ > 0
            // update previous event indicators
            getEventIndicators(comp, comp->prez, NZ);
#endif

#if FMI_VERSION == 3
            if (comp->intermediateUpdate) {

                comp->state = IntermediateUpdateMode;

                int earlyReturnRequested;
                double earlyReturnTime;

                comp->intermediateUpdate((fmi3InstanceEnvironment)comp->componentEnvironment,
                                          comp->time,         // intermediateUpdateTime
                                          1,                  // eventOccurred
                                          comp->clocksTicked, // clocksTicked
                                          0,                  // intermediateVariableSetAllowed
                                          1,                  // intermediateVariableGetAllowed
                                          0,                  // intermediateStepFinished
                                          1,                  // canReturnEarly
                                          &earlyReturnRequested,
                                          &earlyReturnTime);

                comp->state = StepMode;

                if (earlyReturnRequested) {
                    *earlyReturn = 1;
                    // TODO: continue to earlyReturnTime?
                    return status;
                }
            }
#endif
        }

        // terminate simulation, if requested by the model in the previous step
        if (comp->terminateSimulation) {
#if FMI_VERSION == 2
            comp->state = StepFailed;
#endif
            return Discard; // enforce termination of the simulation loop
        }

        comp->time = FIXED_SOLVER_STEP * (++comp->nSteps);

#if FMI_VERSION == 3
        if (comp->intermediateUpdate) {

            comp->state = IntermediateUpdateMode;

            int earlyReturnRequested;
            double earlyReturnTime;

            // call intermediate update callback
            comp->intermediateUpdate((fmi3InstanceEnvironment)comp->componentEnvironment,
                                      comp->time, // intermediateUpdateTime
                                      0,          // eventOccurred
                                      0,          // clocksTicked
                                      0,          // intermediateVariableSetAllowed
                                      1,          // intermediateVariableGetAllowed
                                      1,          // intermediateStepFinished
                                      1,          // canReturnEarly
                                      &earlyReturnRequested,
                                      &earlyReturnTime);

            comp->state = StepMode;
        }
#endif
    }

    if (earlyReturn) {
        *earlyReturn = 0;
    }

    if (lastSuccessfulTime) {
        *lastSuccessfulTime = comp->time;
    }

    return status;
}
