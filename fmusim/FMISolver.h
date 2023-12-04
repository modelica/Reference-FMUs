#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct FMISolverImpl FMISolver;

typedef enum {
    FMISolverOK,
    FMISolverError
} FMISolverStatus;

typedef int  (*FMISolverSetTime)(void* modelInstance, double time);
typedef int  (*FMISolverApplyInput)(void* modelInstance, const void* input, double time, bool discrete, bool continuous, bool afterEvent);
typedef int  (*FMISolverGetContinuousStates)(void* modelInstance, double x[], size_t nx);
typedef int  (*FMISolverSetContinuousStates)(void* modelInstance, const double x[], size_t nx);
typedef int  (*FMISolverGetNominalsOfContinuousStates)(void* modelInstance, double nominals[], size_t nx);
typedef int  (*FMISolverGetContinuousStateDerivatives)(void* modelInstance, double dx[], size_t nx);
typedef int  (*FMISolverGetEventIndicators)(void* modelInstance, double z[], size_t nz);
typedef void (*FMISolverLogError)(const char* message);

typedef struct {

    void* modelInstance;
    const void* input;
    double startTime;
    double tolerance;
    size_t nx;
    size_t nz;
    FMISolverSetTime setTime;
    FMISolverApplyInput applyInput;
    FMISolverGetContinuousStates getContinuousStates;
    FMISolverSetContinuousStates setContinuousStates;
    FMISolverGetNominalsOfContinuousStates getNominalsOfContinuousStates;
    FMISolverGetContinuousStateDerivatives getContinuousStateDerivatives;
    FMISolverGetEventIndicators getEventIndicators;
    FMISolverLogError logError;

} FMISolverParameters;

typedef FMISolver* (*SolverCreate)(const FMISolverParameters* solverFunctions);

typedef void (*SolverFree)(FMISolver* solver);

typedef FMISolverStatus (*SolverStep)(FMISolver* solver, double nextTime, double* timeReached, bool* stateEvent);

typedef FMISolverStatus (*SolverReset)(FMISolver* solver, double time);
