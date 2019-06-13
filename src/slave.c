/****************************************************************
 *  Copyright (c) Dassault Systemes. All rights reserved.       *
 *  This file is part of the Test-FMUs. See LICENSE.txt in the  *
 *  project root for license information.                       *
 ****************************************************************/

#include <string.h>
#include "config.h"
#include "slave.h"

ModelInstance *createModelInstance(
	loggerType cbLogger,
	allocateMemoryType cbAllocateMemory,
	freeMemoryType cbFreeMemory,
	void *componentEnvironment,
	const char *instanceName,
	const char *GUID,
	const char *resourceLocation,
	bool loggingOn,
	InterfaceType interfaceType) {

	ModelInstance *comp = NULL;

	if (!cbLogger) {
		return NULL;
	}

	if (!cbAllocateMemory || !cbFreeMemory) {
		cbLogger(componentEnvironment, instanceName, Error, "error", "Missing callback function.");
		return NULL;
	}

	if (!instanceName || strlen(instanceName) == 0) {
		cbLogger(componentEnvironment, "?", Error, "error", "Missing instance name.");
		return NULL;
	}

	if (!GUID || strlen(GUID) == 0) {
		cbLogger(componentEnvironment, instanceName, Error, "error", "Missing GUID.");
		return NULL;
	}

	if (strcmp(GUID, MODEL_GUID)) {
		cbLogger(componentEnvironment, instanceName, Error, "error", "Wrong GUID.");
		return NULL;
	}

#if FMI_VERSION < 3
	comp = (ModelInstance *)cbAllocateMemory(1, sizeof(ModelInstance));
#else
	comp = (ModelInstance *)cbAllocateMemory(componentEnvironment, 1, sizeof(ModelInstance));
#endif

	if (comp) {

		// set the callbacks
		comp->componentEnvironment = componentEnvironment;
		comp->logger = cbLogger;
		comp->allocateMemory = cbAllocateMemory;
		comp->freeMemory = cbFreeMemory;

		int i;
		comp->instanceName = (char *)allocateMemory(comp, 1 + strlen(instanceName), sizeof(char));

		// resourceLocation is NULL for FMI 1.0 ME
		if (resourceLocation) {
			comp->resourceLocation = (char *)allocateMemory(comp, 1 + strlen(resourceLocation), sizeof(char));
			strcpy((char *)comp->resourceLocation, (char *)resourceLocation);
		} else {
			comp->resourceLocation = NULL;
		}

		comp->modelData = (ModelData *)allocateMemory(comp, 1, sizeof(ModelData));

		// set all categories to on or off. fmi2SetDebugLogging should be called to choose specific categories.
		for (i = 0; i < NUMBER_OF_CATEGORIES; i++) {
			comp->logCategories[i] = loggingOn;
		}
	}

	if (!comp || !comp->modelData || !comp->instanceName) {
		logError(comp, "Out of memory.");
		return NULL;
	}

	comp->time = 0; // overwrite in fmi*SetupExperiment, fmi*SetTime
	strcpy((char *)comp->instanceName, (char *)instanceName);
	comp->type = interfaceType;

	comp->loggingOn = loggingOn;
	comp->state = modelInstantiated;
	comp->isNewEventIteration = false;

	comp->newDiscreteStatesNeeded = false;
	comp->terminateSimulation = false;
	comp->nominalsOfContinuousStatesChanged = false;
	comp->valuesOfContinuousStatesChanged = false;
	comp->nextEventTimeDefined = false;
	comp->nextEventTime = 0;

	setStartValues(comp); // to be implemented by the includer of this file
	comp->isDirtyValues = true; // because we just called setStartValues

#if NUMBER_OF_EVENT_INDICATORS > 0
	comp->z = allocateMemory(comp, sizeof(double), NUMBER_OF_EVENT_INDICATORS);
	comp->prez = allocateMemory(comp, sizeof(double), NUMBER_OF_EVENT_INDICATORS);
#else
	comp->z = NULL;
	comp->prez = NULL;
#endif

	return comp;
}

void freeModelInstance(ModelInstance *comp) {
	freeMemory(comp, (void *)comp->instanceName);
	freeMemory(comp, (void *)comp->z);
	freeMemory(comp, (void *)comp->prez);
	freeMemory(comp, comp);
}

void *allocateMemory(ModelInstance *comp, size_t num, size_t size) {
#if FMI_VERSION > 2
	return comp->allocateMemory(comp->componentEnvironment, num, size);
#else
	return comp->allocateMemory(num, size);
#endif
}

void freeMemory(ModelInstance *comp, void *obj) {
#if FMI_VERSION > 2
	comp->freeMemory(comp->componentEnvironment, obj);
#else
	comp->freeMemory(obj);
#endif
}

const char *duplicateString(ModelInstance *comp, const char *str1) {
	size_t len = strlen(str1);
	char *str2 = allocateMemory(comp, len + 1, sizeof(char));
	strncpy(str2, str1, len + 1);
	return str2;
}

bool invalidNumber(ModelInstance *comp, const char *f, const char *arg, int n, int nExpected) {
	
	if (n != nExpected) {
		comp->state = modelError;
		logError(comp, "%s: Invalid argument %s = %d. Expected %d.", f, arg, n, nExpected);
		return true;
	}
	
	return false;
}

bool invalidState(ModelInstance *comp, const char *f, int statesExpected) {
	
	if (!comp) {
		return true;
	}

	if (!(comp->state & statesExpected)) {
		comp->state = modelError;
		logError(comp, "%s: Illegal call sequence.", f);
		return true;
	}
	
	return false;
}

bool nullPointer(ModelInstance* comp, const char *f, const char *arg, const void *p) {

	if (!p) {
		comp->state = modelError;
		logError(comp, "%s: Invalid argument %s = NULL.", f, arg);
		return true;
	}
	
	return false;
}

// default implementations
#if NUMBER_OF_EVENT_INDICATORS < 1
void getEventIndicators(ModelInstance *comp, double z[], size_t nz) {
    // do nothing
}
#endif

#ifndef GET_FLOAT64
Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    return Error;
}
#endif

#ifndef GET_INT32
Status getInt32(ModelInstance* comp, ValueReference vr, int *value, size_t *index) {
    return Error;
}
#endif

#ifndef GET_BOOLEAN
Status getBoolean(ModelInstance* comp, ValueReference vr, bool *value, size_t *index) {
    return Error;
}
#endif

#ifndef GET_STRING
Status getString(ModelInstance* comp, ValueReference vr, const char **value, size_t *index) {
    return Error;
}
#endif

#ifndef SET_FLOAT64
Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    return Error;
}
#endif

#ifndef SET_INT32
Status setInt32(ModelInstance* comp, ValueReference vr, const int *value, size_t *index) {
    return Error;
}
#endif

#ifndef SET_BOOLEAN
Status setBoolean(ModelInstance* comp, ValueReference vr, const bool *value, size_t *index) {
    return Error;
}
#endif

#ifndef SET_STRING
Status setString(ModelInstance* comp, ValueReference vr, const char *const *value, size_t *index) {
    return Error;
}
#endif

#if NUMBER_OF_STATES < 1
void getContinuousStates(ModelInstance *comp, double x[], size_t nx) {}
void setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {}
void getDerivatives(ModelInstance *comp, double dx[], size_t nx) {}
#endif

Status doStep(ModelInstance *comp, double t, double tNext) {

    int stateEvent = 0;
    int timeEvent  = 0;
	double *temp = NULL;

#if NUMBER_OF_STATES > 0
	double  x[NUMBER_OF_STATES] = { 0 };
	double dx[NUMBER_OF_STATES] = { 0 };
#endif

    while (comp->time + FIXED_SOLVER_STEP < tNext + 0.1 * FIXED_SOLVER_STEP) {
        
#if NUMBER_OF_STATES > 0
		getContinuousStates(comp, x, NUMBER_OF_STATES);
		getDerivatives(comp, dx, NUMBER_OF_STATES);

		// forward Euler step
		for (int i = 0; i < NUMBER_OF_STATES; i++) {
			x[i] += FIXED_SOLVER_STEP * dx[i];
		}

		setContinuousStates(comp, x, NUMBER_OF_STATES);
#endif

		stateEvent = 0;

#if NUMBER_OF_EVENT_INDICATORS > 0
		getEventIndicators(comp, comp->z, NUMBER_OF_EVENT_INDICATORS);
		
		// check for zero-crossing
		for (int i = 0; i < NUMBER_OF_EVENT_INDICATORS; i++) {
		    stateEvent |= (comp->prez[i] * comp->z[i]) <= 0;
		}
		
		// remember the current event indicators
		temp = comp->z;
		comp->z = comp->prez;
		comp->prez = temp;
#endif

        // check for time event
        if (comp->nextEventTimeDefined && (comp->time >= comp->nextEventTime)) {
            timeEvent = 1;
        }

        if (stateEvent || timeEvent) {
            eventUpdate(comp);
            timeEvent  = 0;
            stateEvent = 0;
        }

        // terminate simulation, if requested by the model in the previous step
        if (comp->terminateSimulation) {
#if FMI_VERSION > 1
            comp->state = modelStepFailed;
#endif
            return Discard; // enforce termination of the simulation loop
        }

		comp->time += FIXED_SOLVER_STEP;
    }

    return OK;
}
