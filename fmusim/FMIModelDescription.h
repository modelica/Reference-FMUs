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

typedef enum {

    FMIConstant, 
    FMIFixed, 
    FMITunable, 
    FMIDiscrete, 
    FMIContinuous

} FMIVariability;

typedef enum {

    FMIFlat,
    FMIStructured

} FMIVariableNamingConvention;

typedef enum {

    FMIUndefined,
    FMIExact,
    FMIApprox,
    FMICalculated

} FMIInitial;

typedef struct {

    const char* name;
    int kg;
    int m;
    int s;
    int A;
    int K;
    int mol;
    int cd;
    int rad;
    double factor;
    double offset;

} FMIBaseUnit;

typedef struct {

    const char* name;
    double factor;
    double offset;

} FMIDisplayUnit;

typedef struct {

    const char* name;
    FMIBaseUnit* baseUnit;
    size_t nDisplayUnits;
    FMIDisplayUnit** displayUnits;

} FMIUnit;

typedef struct {

    FMIVariableType type;
    const char* name;
    const char* quantity;
    FMIUnit* unit;
    FMIDisplayUnit* displayUnit;
    bool relativeQuantity;
    const char* min;
    const char* max;
    const char* nominal;
    bool unbounded;

} FMITypeDefinition;

typedef struct FMIDimension FMIDimension;

typedef struct FMIModelVariable FMIModelVariable;

struct FMIModelVariable {
    
    FMIVariableType type;
    const char* name;
    const char* min;
    const char* max;
    const char* nominal;
    const char* start;
    const char* description;
    unsigned int valueReference;
    FMICausality causality;
    FMIVariability variability;
    FMIInitial initial;
    size_t nDimensions;
    FMIDimension* dimensions;
    FMIModelVariable* derivative;
    FMIUnit* unit;
    bool relativeQuantity;
    FMITypeDefinition* declaredType;
    unsigned short line;

};

struct FMIDimension{

    size_t start;
    FMIModelVariable* variable;

};

typedef struct {

    const char* modelIdentifier;
    bool providesDirectionalDerivatives;
    bool needsCompletedIntegratorStep;

} FMIModelExchangeInterface;

typedef struct {

    const char* modelIdentifier;
    bool hasEventMode;

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

    FMIMajorVersion fmiMajorVersion;
    const char* fmiVersion;
    const char* modelName;
    const char* instantiationToken;
    const char* description;
    const char* generationTool;
    const char* generationDate;
    FMIVariableNamingConvention variableNamingConvention;

    FMIModelExchangeInterface* modelExchange;
    FMICoSimulationInterface* coSimulation;

    FMIDefaultExperiment* defaultExperiment;

    size_t nUnits;
    FMIUnit** units;

    size_t nTypeDefinitions;
    FMITypeDefinition** typeDefinitions;

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

FMIUnit* FMIUnitForName(const FMIModelDescription* modelDescription, const char* name);

FMIDisplayUnit* FMIDisplayUnitForName(const FMIUnit* unit, const char* name);

FMITypeDefinition* FMITypeDefintionForName(const FMIModelDescription* modelDescription, const char* name);

FMIModelVariable* FMIModelVariableForName(const FMIModelDescription* modelDescription, const char* name);

FMIModelVariable* FMIModelVariableForValueReference(const FMIModelDescription* modelDescription, FMIValueReference valueReference);

FMIModelVariable* FMIModelVariableForIndexLiteral(const FMIModelDescription* modelDescription, const char* index);

size_t FMIValidateModelDescription(const FMIModelDescription* modelDescription);

void FMIDumpModelDescription(FMIModelDescription* modelDescription, FILE* file);
