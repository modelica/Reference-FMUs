#pragma once

#include "FMI.h"
#include "FMIModelDescription.h"


FMIStatus FMIGetValues(
    FMIInstance* instance,
    FMIVariableType type,
    const FMIValueReference valueReferences[],
    size_t nValueReferences,
    size_t sizes[],
    void* values,
    size_t nValues);

FMIStatus FMISetValues(
    FMIInstance* instance,
    FMIVariableType type,
    const FMIValueReference valueReferences[],
    size_t nValueReferences,
    const size_t sizes[],
    const void* values,
    size_t nValues);

FMIStatus FMIGetNumberOfVariableValues(
    FMIInstance* instance, 
    const FMIModelVariable* variable, 
    size_t* nValues);

FMIStatus FMIGetNumberOfUnkownValues(
    FMIInstance* instance,
    size_t nUnknowns,
    const FMIUnknown* unknowns[],
    size_t* nValues);

size_t FMISizeOfVariableType(FMIVariableType type, FMIMajorVersion majorVersion);

FMIStatus FMIParseValues(FMIMajorVersion fmiMajorVersion, FMIVariableType type, const char* literal, size_t* nValues, void** values);

FMIStatus FMIParseStartValues(FMIVariableType type, const char* literal, size_t nValues, void* values);

FMIStatus FMIHexToBinary(const char* hex, size_t size, unsigned char* binary);

FMIStatus FMIRestoreFMUStateFromFile(FMIInstance* S, const char* filename);

FMIStatus FMISaveFMUStateToFile(FMIInstance* S, const char* filename);

FMIStatus FMIDuplicateString(const char* source, char** destination);

FMIStatus FMIDuplicateBuffer(const void* source, void** destination, size_t size);

bool FMIIsClose(double a, double b);