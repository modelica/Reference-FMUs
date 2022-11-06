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

    FMIVersion fmiVersion;
    const char* modelIdentifier;
    const char* instantiationToken;
    const char* description;
    const char* generationTool;
    const char* generationDate;

    size_t nModelVariables;
    FMIModelVariable* modelVariables;

    size_t nOutputs;
    size_t nContinuousStates;
    size_t nInitialUnknowns;
    size_t nEventIndicators;

} FMIModelDescription;

FMIModelDescription* FMIReadModelDescription(const char* filename);

void FMIFreeModelDescription(FMIModelDescription* modelDescription);

void FMIDumpModelDescription(FMIModelDescription* modelDescription, FILE* file);

#endif  // FMI_MODEL_DESCRIPTION_H