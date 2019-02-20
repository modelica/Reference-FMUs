#ifndef model_h
#define model_h

#include <stddef.h>  // for size_t
#include <stdbool.h> // for bool

#include "config.h"

// categories of logging supported by model.
// Value is the index in logCategories of a ModelInstance.
#define LOG_ALL       0
#define LOG_ERROR     1
#define LOG_FMI_CALL  2
#define LOG_EVENT     3

#define NUMBER_OF_CATEGORIES 4

#if FMI_VERSION == 1

#define not_modelError (modelInstantiated|modelInitialized|modelTerminated)

typedef enum {
    modelInstantiated = 1<<0,
    modelInitialized  = 1<<1,
    modelTerminated   = 1<<2,
    modelError        = 1<<3
} ModelState;

#else

typedef enum {
    modelStartAndEnd        = 1<<0,
    modelInstantiated       = 1<<1,
    modelInitializationMode = 1<<2,

    // ME states
    modelEventMode          = 1<<3,
    modelContinuousTimeMode = 1<<4,
    
    // CS states
    modelStepComplete       = 1<<5,
    modelStepInProgress     = 1<<6,
    modelStepFailed         = 1<<7,
    modelStepCanceled       = 1<<8,

    modelTerminated         = 1<<9,
    modelError              = 1<<10,
    modelFatal              = 1<<11,
} ModelState;

#endif

typedef enum {
    ModelExchange,
    CoSimulation
} InterfaceType;

typedef enum {
    OK,
    Warning,
    Discard,
    Error,
    Fatal,
    Pending
} Status;


typedef struct {
    
    double time;
    const char *instanceName;
    InterfaceType type;
    const char *GUID;
    const char *resourceLocation;

    // callback functions
#if FMI_VERSION < 3
    void  (*logger)(void *, const char *, int, const char *, const char *, ...);
    void* (*allocateMemory)(size_t, size_t);
    void  (*freeMemory)(void *);
    void  (*stepFinished)(void *, int);
#else
    void  (*logger)(void *, const char *, int, const char *, const char *, ...);
    void* (*allocateMemory)(void *, size_t, size_t);
    void  (*freeMemory)(void *, void *);
    void  (*stepFinished)(void *, void *, int);
#endif
    bool loggingOn;
    bool logCategories[NUMBER_OF_CATEGORIES];

    void *componentEnvironment;
    ModelState state;
    
    // event info
    bool newDiscreteStatesNeeded;
    bool terminateSimulation;
    bool nominalsOfContinuousStatesChanged;
    bool valuesOfContinuousStatesChanged;
    bool nextEventTimeDefined;
    double nextEventTime;
    
    bool isDirtyValues;
    bool isNewEventIteration;
    
    ModelData *modelData;
    void *solverData;
    
} ModelInstance;

// shorthand to access the variables
#define M(v) (comp->modelData->v)

void setStartValues(ModelInstance *comp);
void calculateValues(ModelInstance *comp);
    
Status getReal(ModelInstance* comp, ValueReference vr, double *value);
Status getInteger(ModelInstance* comp, ValueReference vr, int *value);
Status getBoolean(ModelInstance* comp, ValueReference vr, bool *value);

Status setReal(ModelInstance* comp, ValueReference vr, double value);
Status setInteger(ModelInstance* comp, ValueReference vr, int value);
Status setBoolean(ModelInstance* comp, ValueReference vr, bool value);

void getContinuousStates(ModelInstance *comp, double x[], size_t nx);
void setContinuousStates(ModelInstance *comp, const double x[], size_t nx);
void getDerivatives(ModelInstance *comp, double dx[], size_t nx);
void getEventIndicators(ModelInstance *comp, double z[], size_t nz);
void eventUpdate(ModelInstance *comp);

void logError(ModelInstance *comp, const char *message, ...);

#endif  /* model_h */
