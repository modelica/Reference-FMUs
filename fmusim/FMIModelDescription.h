#pragma once

#include <stdio.h>
#include <stdbool.h>

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

typedef struct FMIModelVariable FMIModelVariable;

struct FMIModelVariable {
    
    const char* name;
    FMIVariableType type;
    const char* description;
    unsigned int valueReference;
    FMICausality causality;
    size_t nDimensions;
    FMIDimension* dimensions;
    FMIModelVariable* derivative;

};

struct FMIDimension{

    size_t start;
    FMIModelVariable* variable;

};

typedef struct {

    const char* modelIdentifier;
    bool providesDirectionalDerivatives;

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

    FMIModelVariable* modelVariable;

} FMIUnknown;

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
    FMIUnknown* outputs;

    size_t nContinuousStates;
    FMIUnknown* derivatives;

    size_t nInitialUnknowns;
    FMIUnknown* initialUnknowns;

    size_t nEventIndicators;
    FMIUnknown* eventIndicators;

} FMIModelDescription;

FMIModelDescription* FMIReadModelDescription(const char* filename);

void FMIFreeModelDescription(FMIModelDescription* modelDescription);

FMIValueReference FMIValueReferenceForLiteral(const char* literal);

FMIModelVariable* FMIModelVariableForName(const FMIModelDescription* modelDescription, const char* name);

FMIModelVariable* FMIModelVariableForValueReference(const FMIModelDescription* modelDescription, FMIValueReference valueReference);

FMIModelVariable* FMIModelVariableForIndexLiteral(const FMIModelDescription* modelDescription, const char* index);

size_t FMIValidateModelStructure(const FMIModelDescription* modelDescription);

void FMIDumpModelDescription(FMIModelDescription* modelDescription, FILE* file);
