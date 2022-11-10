#ifndef FMI_MODEL_DESCRIPTION_H
#define FMI_MODEL_DESCRIPTION_H

#include "FMI.h"
#include <stdio.h>

typedef enum {
    FMIParameter,
    FMICalculatedParameter,
    FMIStructuralParameter,
    FMIInput,
    FMIOutput,
    FMILocal,
    FMIIndependent
} FMICausality;

typedef struct {
    
    const char* name;
    FMIVariableType type;
    const char* description;
    unsigned int valueReference;
    FMICausality causality;

} FMIModelVariable;

typedef struct {

    const char* modelIdentifier;

} FMIModelExchangeInterface;

typedef struct {

    const char* modelIdentifier;

} FMICoSimulationInterface;

typedef struct {

    const char* startTime;
    const char* stopTime;
    const char* stepSize;

} FMIDefaultExperiment;

typedef struct {

    FMIVersion fmiVersion;
    const char* modelName;
    const char* instantiationToken;
    const char* description;
    const char* generationTool;
    const char* generationDate;

    FMIModelExchangeInterface* modelExchange;
    FMICoSimulationInterface* coSimulation;

    FMIDefaultExperiment* defaultExperiment;

    size_t nModelVariables;
    FMIModelVariable* modelVariables;

    size_t nOutputs;
    size_t nContinuousStates;
    size_t nInitialUnknowns;
    size_t nEventIndicators;

} FMIModelDescription;

FMIModelDescription* FMIReadModelDescription(const char* filename);

void FMIFreeModelDescription(FMIModelDescription* modelDescription);

FMIModelVariable* FMIModelVariableForName(const FMIModelDescription* modelDescription, const char* name);

void FMIDumpModelDescription(FMIModelDescription* modelDescription, FILE* file);

#endif  // FMI_MODEL_DESCRIPTION_H
