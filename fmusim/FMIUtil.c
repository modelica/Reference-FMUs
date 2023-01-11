#include <string.h>

#include "FMI1.h"
#include "FMI2.h"
#include "FMI3.h"

#include "FMIUtil.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


 FMIStatus FMIGetNumberOfVariableValues(
     FMIInstance* instance, 
     const FMIModelVariable* variable, 
     size_t* nValues) {

    FMIStatus status = FMIOK;

    *nValues = 1;

    if (variable->nDimensions > 0) {

        for (size_t j = 0; j < variable->nDimensions; j++) {

            const FMIDimension* dimension = &variable->dimensions[j];

            fmi3UInt64 extent;

            if (dimension->variable) {
                CALL(FMI3GetUInt64(instance, &dimension->variable->valueReference, 1, &extent, 1));
            } else {
                extent = dimension->start;
            }

            *nValues *= extent;
        }

    }

TERMINATE:
    return status;
}

FMIStatus FMI1SetValues(
     FMIInstance* instance,
     FMIVariableType type,
     const FMIValueReference valueReferences[],
     size_t nValueReferences,
     const void* values) {

     switch (type) {
     case FMIRealType:
         return FMI1SetReal(instance, valueReferences, nValueReferences, (fmi1Real*)values);
     case FMIIntegerType:
         return FMI1SetInteger(instance, valueReferences, nValueReferences, (fmi1Integer*)values);
     case FMIBooleanType:
         return FMI1SetBoolean(instance, valueReferences, nValueReferences, (fmi1Boolean*)values);
     case FMIStringType:
         return FMI1SetString(instance, valueReferences, nValueReferences, (fmi1String*)values);
     default:
         return FMIError;
     }
 }

FMIStatus FMI2SetValues(
    FMIInstance* instance,
    FMIVariableType type,
    const FMIValueReference valueReferences[],
    size_t nValueReferences,
    const void* values) {

    switch (type) {
    case FMIRealType:
        return FMI2SetReal(instance, valueReferences, nValueReferences, (fmi2Real*)values);
    case FMIIntegerType:
        return FMI2SetInteger(instance, valueReferences, nValueReferences, (fmi2Integer*)values);
    case FMIBooleanType:
        return FMI2SetBoolean(instance, valueReferences, nValueReferences, (fmi2Boolean*)values);
    case FMIStringType:
        return FMI2SetString(instance, valueReferences, nValueReferences, (fmi2String*)values);
    default:
        return FMIError;
    }
}

FMIStatus FMI3SetValues(
    FMIInstance* instance,
    FMIVariableType type,
    const FMIValueReference valueReferences[],
    size_t nValueReferences,
    const void* values,
    size_t nValues) {

    switch (type) {
    case FMIFloat32Type:
        return FMI3SetFloat32(instance, valueReferences, nValueReferences, (fmi3Float32*)values, nValues);
    case FMIFloat64Type:
        return FMI3SetFloat64(instance, valueReferences, nValueReferences, (fmi3Float64*)values, nValues);
    case FMIInt8Type:
        return FMI3SetInt8(instance, valueReferences, nValueReferences, (fmi3Int8*)values, nValues);
    case FMIUInt8Type:
        return FMI3SetUInt8(instance, valueReferences, nValueReferences, (fmi3UInt8*)values, nValues);
    case FMIInt16Type:
        return FMI3SetInt16(instance, valueReferences, nValueReferences, (fmi3Int16*)values, nValues);
    case FMIUInt16Type:
        return FMI3SetUInt16(instance, valueReferences, nValueReferences, (fmi3UInt16*)values, nValues);
    case FMIInt32Type:
        return FMI3SetInt32(instance, valueReferences, nValueReferences, (fmi3Int32*)values, nValues);
    case FMIUInt32Type:
        return FMI3SetUInt16(instance, valueReferences, nValueReferences, (fmi3UInt32*)values, nValues);
    case FMIInt64Type:
        return FMI3SetInt64(instance, valueReferences, nValueReferences, (fmi3Int64*)values, nValues);
    case FMIUInt64Type:
        return FMI3SetUInt64(instance, valueReferences, nValueReferences, (fmi3UInt64*)values, nValues);
    case FMIBooleanType:
        return FMI3SetBoolean(instance, valueReferences, nValueReferences, (fmi3Boolean*)values, nValues);
    case FMIStringType:
        return FMI3SetString(instance, valueReferences, nValueReferences, (fmi3String*)values, nValues);
    case FMIBinaryType:
        return FMIError;
    case FMIClockType:
        return FMI3SetClock(instance, valueReferences, nValueReferences, (fmi3Clock*)values);
    default:
        return FMIError;
    }
 }

 FMIStatus FMIParseStartValues(FMIVariableType type, const char* literal, size_t nValues, void* values) {

     if (type == FMIFloat32Type || type == FMIDiscreteFloat32Type) {

         fmi3Float32* v = (fmi3Float32*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtof(next, &next);
         }

     } else if (type == FMIFloat64Type || type == FMIDiscreteFloat64Type) {

         fmi3Float64* v = (fmi3Float64*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtod(next, &next);
         }

     } else if (type == FMIInt8Type) {

         fmi3Int8* v = (fmi3Int8*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtol(next, &next, 10);
         }

     } else if (type == FMIUInt8Type) {

         fmi3UInt8* v = (fmi3UInt8*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoul(next, &next, 10);
         }

     } else if (type == FMIInt16Type) {

         fmi3Int16* v = (fmi3Int16*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtol(next, &next, 10);
         }

     } else if (type == FMIUInt16Type) {

         fmi3UInt16* v = (fmi3UInt16*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoul(next, &next, 10);
         }

     } else if (type == FMIInt32Type) {

         fmi3Int32* v = (fmi3Int32*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtol(next, &next, 10);
         }

     } else if (type == FMIUInt32Type) {

         fmi3UInt32* v = (fmi3UInt32*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoul(next, &next, 10);
         }

     } else if (type == FMIInt64Type) {

         fmi3Int64* v = (fmi3Int64*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtol(next, &next, 10);
         }

     } else if (type == FMIUInt64Type) {

         fmi3UInt64* v = (fmi3UInt64*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoul(next, &next, 10);
         }

     } else if (type == FMIBooleanType) {

         fmi3Boolean* v = (fmi3Boolean*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoul(next, &next, 10);
         }

     } else if (type == FMIStringType) {

         return FMIError;

     } else if (type == FMIBinaryType) {

         return FMIError;

     } else if (type == FMIClockType) {

         fmi3Clock* v = (fmi3Clock*)values;
         char* next = literal;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoul(next, &next, 10);
         }

     }

     return FMIOK;
 }

 static int hexchr2bin(const char hex, char* out) {

     if (out == NULL) {
         return 0;
     }

     if (hex >= '0' && hex <= '9') {
         *out = hex - '0';
     } else if (hex >= 'A' && hex <= 'F') {
         *out = hex - 'A' + 10;
     } else if (hex >= 'a' && hex <= 'f') {
         *out = hex - 'a' + 10;
     } else {
         return 0;
     }

     return 1;
 }

 FMIStatus FMIHexToBinary(const char* hex, size_t* size, unsigned char** value) {

     if (hex == NULL || *hex == '\0' || value == NULL) {
         return FMIError;
     }
         
     *size = strlen(hex);

     if (*size % 2 != 0) {
         return FMIError;
     }

     *size /= 2;

     *value = malloc(*size);

     if (!*value) {
         return FMIError;
     }

     memset(*value, 'A', *size);

     for (size_t i = 0; i < *size; i++) {
         char b1, b2;
         if (!hexchr2bin(hex[i * 2], &b1) || !hexchr2bin(hex[i * 2 + 1], &b2)) {
             return 0;
         }
         (*value)[i] = (b1 << 4) | b2;
     }

     return FMIOK;
 }
