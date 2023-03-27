#pragma once

#include "FMI.h"
#include "FMIModelDescription.h"


FMIStatus FMIGetNumberOfVariableValues(
    FMIInstance* instance, 
    const FMIModelVariable* variable, 
    size_t* nValues);

FMIStatus FMI1SetValues(
    FMIInstance* instance,
    FMIVariableType type,
    const FMIValueReference valueReferences[],
    size_t nValueReferences,
    const void* values);

FMIStatus FMI2SetValues(
    FMIInstance* instance,
    FMIVariableType type,
    const FMIValueReference valueReferences[],
    size_t nValueReferences,
    const void* values);

FMIStatus FMI3SetValues(
    FMIInstance* instance,
    FMIVariableType type,
    const FMIValueReference valueReferences[],
    size_t nValueReferences,
    const void* values,
    size_t nValues);

FMIStatus FMIParseStartValues(FMIVariableType type, const char* literal, size_t nValues, void* values);

FMIStatus FMIHexToBinary(const char* hex, size_t* size, unsigned char** value);

FMIStatus FMIRestoreFMUStateFromFile(FMIInstance* S, const char* filename);

FMIStatus FMISaveFMUStateToFile(FMIInstance* S, const char* filename);
