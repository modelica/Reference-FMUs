#pragma once

#include <stdio.h>

#include "FMI.h"


typedef enum {

    FMIParameter,
    FMICalculatedParameter,
    FMIStructuralParameter,
    FMIInput,
    FMIOutput,
    FMILocal,
    FMIIndependent

} FMICausality;

typedef struct FMIDimension FMIDimension;

typedef struct {
    
    const char* name;
    FMIVariableType type;
    const char* description;
    unsigned int valueReference;
    FMICausality causality;
    size_t nDimensions;
    FMIDimension* dimensions;

} FMIModelVariable;

struct FMIDimension{

    size_t start;
    FMIModelVariable* variable;

};

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

FMIModelVariable* FMIModelVariableForValueReference(const FMIModelDescription* modelDescription, FMIValueReference valueReference);

void FMIDumpModelDescription(FMIModelDescription* modelDescription, FILE* file);
