#include <string.h>
#include <math.h>

#include "FMI1.h"
#include "FMI2.h"
#include "FMI3.h"

#include "FMIUtil.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus FMIGetValues(
    FMIInstance* instance,
    FMIVariableType type,
    const FMIValueReference valueReferences[],
    size_t nValueReferences,
    size_t sizes[],
    void* values,
    size_t nValues) {

    switch (instance->fmiMajorVersion) {

    case FMIMajorVersion1:

        switch (type) {
        case FMIRealType:
            return FMI1GetReal(instance, valueReferences, nValueReferences, (fmi1Real*)values);
        case FMIIntegerType:
            return FMI1GetInteger(instance, valueReferences, nValueReferences, (fmi1Integer*)values);
        case FMIBooleanType:
            return FMI1GetBoolean(instance, valueReferences, nValueReferences, (fmi1Boolean*)values);
        case FMIStringType:
            return FMI1GetString(instance, valueReferences, nValueReferences, (fmi1String*)values);
        default:
            return FMIError;
        }

        break;

    case FMIMajorVersion2:

        switch (type) {
        case FMIRealType:
            return FMI2GetReal(instance, valueReferences, nValueReferences, (fmi2Real*)values);
        case FMIIntegerType:
            return FMI2GetInteger(instance, valueReferences, nValueReferences, (fmi2Integer*)values);
        case FMIBooleanType:
            return FMI2GetBoolean(instance, valueReferences, nValueReferences, (fmi2Boolean*)values);
        case FMIStringType:
            return FMI2GetString(instance, valueReferences, nValueReferences, (fmi2String*)values);
        default:
            return FMIError;
        }

        break;
    
    case FMIMajorVersion3:

        switch (type) {
        case FMIFloat32Type:
            return FMI3GetFloat32(instance, valueReferences, nValueReferences, (fmi3Float32*)values, nValues);
        case FMIFloat64Type:
            return FMI3GetFloat64(instance, valueReferences, nValueReferences, (fmi3Float64*)values, nValues);
        case FMIInt8Type:
            return FMI3GetInt8(instance, valueReferences, nValueReferences, (fmi3Int8*)values, nValues);
        case FMIUInt8Type:
            return FMI3GetUInt8(instance, valueReferences, nValueReferences, (fmi3UInt8*)values, nValues);
        case FMIInt16Type:
            return FMI3GetInt16(instance, valueReferences, nValueReferences, (fmi3Int16*)values, nValues);
        case FMIUInt16Type:
            return FMI3GetUInt16(instance, valueReferences, nValueReferences, (fmi3UInt16*)values, nValues);
        case FMIInt32Type:
            return FMI3GetInt32(instance, valueReferences, nValueReferences, (fmi3Int32*)values, nValues);
        case FMIUInt32Type:
            return FMI3GetUInt32(instance, valueReferences, nValueReferences, (fmi3UInt32*)values, nValues);
        case FMIInt64Type:
            return FMI3GetInt64(instance, valueReferences, nValueReferences, (fmi3Int64*)values, nValues);
        case FMIUInt64Type:
            return FMI3GetUInt64(instance, valueReferences, nValueReferences, (fmi3UInt64*)values, nValues);
        case FMIBooleanType:
            return FMI3GetBoolean(instance, valueReferences, nValueReferences, (fmi3Boolean*)values, nValues);
        case FMIStringType:
            return FMI3GetString(instance, valueReferences, nValueReferences, (fmi3String*)values, nValues);
        case FMIBinaryType:
            return FMI3GetBinary(instance, valueReferences, nValueReferences, sizes, (fmi3Binary*)values, nValues);
        case FMIClockType:
            return FMI3GetClock(instance, valueReferences, nValueReferences, (fmi3Clock*)values);
        default:
            return FMIError;
        }

        break;
    }

    return FMIError;
}

 FMIStatus FMIGetNumberOfVariableValues(
     FMIInstance* instance, 
     const FMIModelVariable* variable, 
     size_t* nValues) {

    FMIStatus status = FMIOK;

    *nValues = 1;

    if (variable->nDimensions > 0) {

        for (size_t j = 0; j < variable->nDimensions; j++) {

            const FMIDimension* dimension = variable->dimensions[j];

            fmi3UInt64 extent;

            if (dimension->variable) {
                CALL(FMI3GetUInt64(instance, &dimension->variable->valueReference, 1, &extent, 1));
            } else {
                extent = dimension->start;
            }

            if (extent == 0) {
                *nValues = 0;
                goto TERMINATE;
            }

            *nValues *= extent;
        }

    }

TERMINATE:
    return status;
}

FMIStatus FMIGetNumberOfUnkownValues(
    FMIInstance* instance,
    size_t nUnknowns,
    const FMIUnknown* unknowns[],
    size_t* nValues) {

     FMIStatus status = FMIOK;

     *nValues = 0;

     for (size_t i = 0; i < nUnknowns; i++) {

         const FMIUnknown* unknown = unknowns[i];

         size_t n;

         CALL(FMIGetNumberOfVariableValues(instance, unknown->modelVariable, &n));

         *nValues += n;
     }

 TERMINATE:
     return status;
 }

FMIStatus FMISetValues(
    FMIInstance* instance,
    FMIVariableType type,
    const FMIValueReference valueReferences[],
    size_t nValueReferences,
    const size_t sizes[],
    const void* values,
    size_t nValues) {

    switch(instance->fmiMajorVersion) {
    case FMIMajorVersion1:

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

        break;

    case FMIMajorVersion2:

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

        break;

    case FMIMajorVersion3:

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
            return FMI3SetUInt32(instance, valueReferences, nValueReferences, (fmi3UInt32*)values, nValues);
        case FMIInt64Type:
            return FMI3SetInt64(instance, valueReferences, nValueReferences, (fmi3Int64*)values, nValues);
        case FMIUInt64Type:
            return FMI3SetUInt64(instance, valueReferences, nValueReferences, (fmi3UInt64*)values, nValues);
        case FMIBooleanType:
            return FMI3SetBoolean(instance, valueReferences, nValueReferences, (fmi3Boolean*)values, nValues);
        case FMIStringType:
            return FMI3SetString(instance, valueReferences, nValueReferences, (fmi3String*)values, nValues);
        case FMIBinaryType:
            return FMI3SetBinary(instance, valueReferences, nValueReferences, sizes, (fmi3Binary*)values, nValues);
        case FMIClockType:
            return FMI3SetClock(instance, valueReferences, nValueReferences, (fmi3Clock*)values);
        default:
            return FMIError;
        }

        break;
    }

    return FMIError;
}

#define PARSE_VALUES(t, f, ...) \
    while (strlen(next) > 0) { \
        CALL(FMIRealloc(values, sizeof(t) * ((*nValues) + 1))); \
        t* v = (t*)*values; \
        char* end; \
        v[*nValues] = f(next, &end, ##__VA_ARGS__); \
        if (end == next) {  \
            status = FMIError; \
            goto TERMINATE; \
        } \
        next = end; \
        (*nValues)++; \
    }

size_t FMISizeOfVariableType(FMIVariableType type, FMIMajorVersion majorVersion) {

    switch (type) {
    case FMIFloat32Type:        return sizeof(fmi3Float32);
    case FMIFloat64Type:        return sizeof(fmi3Float64);
    case FMIInt8Type:           return sizeof(fmi3Int8);
    case FMIUInt8Type:          return sizeof(fmi3UInt8);
    case FMIInt16Type:          return sizeof(fmi3Int16);
    case FMIUInt16Type:         return sizeof(fmi3UInt16);
    case FMIInt32Type:          return sizeof(fmi3Int32);
    case FMIUInt32Type:         return sizeof(fmi3UInt32);
    case FMIInt64Type:          return sizeof(fmi3Int64);
    case FMIUInt64Type:         return sizeof(fmi3UInt64);
    case FMIBooleanType:
        switch (majorVersion) {
        case FMIMajorVersion1:  return sizeof(fmi1Boolean);
        case FMIMajorVersion2:  return sizeof(fmi2Boolean);
        case FMIMajorVersion3:  return sizeof(fmi3Boolean);
        }
    case FMIStringType:         return sizeof(fmi3String);
    case FMIBinaryType:         return sizeof(fmi3Binary);
    case FMIClockType:          return sizeof(fmi3Clock);
    case FMIValueReferenceType: return sizeof(FMIValueReference);
    case FMISizeTType:          return sizeof(size_t);
    default:                    return 0;
    }
}

FMIStatus FMIParseValues(FMIMajorVersion fmiMajorVersion, FMIVariableType type, const char* literal, size_t* nValues, void** values, size_t** sizes) {

    FMIStatus status = FMIOK;

    if (!literal) {
        FMILogError("Argument literal must not be NULL.\n");
        return FMIError;
    }

    if (!values) {
        FMILogError("Argument values must not be NULL.\n");
        return FMIError;
    }

    *nValues = 0;
    *values = NULL;

    if (sizes) {
        *sizes = NULL;
    }

    char* next = (char*)literal;

    switch (type) {
    case FMIFloat32Type:
        PARSE_VALUES(fmi3Float32, strtof);
        break;
    case FMIFloat64Type:
        PARSE_VALUES(fmi3Float64, strtod);
        break;
    case FMIInt8Type:
        PARSE_VALUES(fmi3Int8, strtol, 10);
        break;
    case FMIUInt8Type:
        PARSE_VALUES(fmi3UInt8, strtoul, 10);
        break;
    case FMIInt16Type:
        PARSE_VALUES(fmi3Int16, strtol, 10);
        break;
    case FMIUInt16Type:
        PARSE_VALUES(fmi3UInt16, strtoul, 10);
        break;
    case FMIInt32Type:
        PARSE_VALUES(fmi3Int32, strtol, 10);
        break;
    case FMIUInt32Type:
        PARSE_VALUES(fmi3UInt32, strtoul, 10);
        break;
    case FMIInt64Type:
        PARSE_VALUES(fmi3Int64, strtoll, 10);
        break;
    case FMIUInt64Type:
        PARSE_VALUES(fmi3UInt64, strtoull, 10);
        break;
    case FMIStringType: {
        const size_t len = strlen(literal);
        CALL(FMIRealloc(values, sizeof(fmi3String*) + len + 1));
        fmi3String* v = *values;
        v[0] = (fmi3String)&v[1];
        strncpy((char*)&v[1], literal, len + 1);
        *nValues = 1;
        break;
    }
    case FMIBooleanType:
    case FMIClockType: {

        const size_t size = FMISizeOfVariableType(FMIBooleanType, fmiMajorVersion);

        while (strlen(next) > 0) {

            bool value;

            size_t delimiter = strcspn(next, " ");

            if (!strncmp(next, "0", delimiter) || !strncmp(next, "false", delimiter)) {
                value = false;
            } else if (!strncmp(next, "1", delimiter) || !strncmp(next, "true", delimiter)) {
                value = true;
            } else {
                FMILogError("Values for Boolean must be one of 0, 1, false, or true.\n");
                status = FMIError;
                goto TERMINATE;
            }

            CALL(FMIRealloc(values, size * ((*nValues) + 1)));

            if (fmiMajorVersion == FMIMajorVersion1) {
                fmi1Boolean* v = (fmi1Boolean*)*values;
                v[*nValues] = value ? fmi1True : fmi1False;
            } else if (fmiMajorVersion == FMIMajorVersion2) {
                fmi2Boolean* v = (fmi2Boolean*)*values;
                v[*nValues] = value ? fmi2True : fmi2False;
            } else if (fmiMajorVersion == FMIMajorVersion3) {
                fmi3Boolean* v = (fmi3Boolean*)*values;
                v[*nValues] = value ? fmi3True : fmi3False;
            }

            (*nValues)++;

            if (strlen(next) == delimiter) {
                break;
            }

            next += delimiter + 1;
        }
        break;
    }
    case FMIBinaryType: {
        const size_t length = strlen(literal);
        if (length % 2 != 0) {
            FMILogError("The length of a binary literal must be a multiple of two.");
            status = FMIError;
            goto TERMINATE;
        }
        const size_t size = length / 2;
        CALL(FMICalloc(values, 1, sizeof(fmi3Binary) + size * sizeof(fmi3Byte)));
        fmi3Binary* v = (fmi3Binary*)*values;
        fmi3Byte* b = (fmi3Byte*)&v[1];
        CALL(FMIHexToBinary(literal, size, b));
        v[0] = b;
        *nValues = 1;
        CALL(FMICalloc(sizes, 1, sizeof(size_t)));
        **sizes = size;
        break;
    }
    default:
        FMILogError("Unsupported value type.");
        status = FMIError;
        goto TERMINATE;
    }

TERMINATE:
    if (status != FMIOK) {
        *nValues = 0;
        free(*values);
        *values = NULL;
        FMILogError("Failed to parse value literal \"%s\".\n", literal);
    }

    return status;
}

 FMIStatus FMIParseStartValues(FMIVariableType type, const char* literal, size_t nValues, void* values) {

     char* next = (char*)literal;

     if (type == FMIFloat32Type) {

         fmi3Float32* v = (fmi3Float32*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtof(next, &next);
         }

     } else if (type == FMIFloat64Type) {

         fmi3Float64* v = (fmi3Float64*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtod(next, &next);
         }

     } else if (type == FMIInt8Type) {

         fmi3Int8* v = (fmi3Int8*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtol(next, &next, 10);
         }

     } else if (type == FMIUInt8Type) {

         fmi3UInt8* v = (fmi3UInt8*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoul(next, &next, 10);
         }

     } else if (type == FMIInt16Type) {

         fmi3Int16* v = (fmi3Int16*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtol(next, &next, 10);
         }

     } else if (type == FMIUInt16Type) {

         fmi3UInt16* v = (fmi3UInt16*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoul(next, &next, 10);
         }

     } else if (type == FMIInt32Type) {

         fmi3Int32* v = (fmi3Int32*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtol(next, &next, 10);
         }

     } else if (type == FMIUInt32Type) {

         fmi3UInt32* v = (fmi3UInt32*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoul(next, &next, 10);
         }

     } else if (type == FMIInt64Type) {

         fmi3Int64* v = (fmi3Int64*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoll(next, &next, 10);
         }

     } else if (type == FMIUInt64Type) {

         fmi3UInt64* v = (fmi3UInt64*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoull(next, &next, 10);
         }

     } else if (type == FMIBooleanType) {

         fmi3Boolean* v = (fmi3Boolean*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtoul(next, &next, 10);
         }

     } else if (type == FMIStringType) {

         return FMIError;

     } else if (type == FMIBinaryType) {

         return FMIError;

     } else if (type == FMIClockType) {

         fmi3Clock* v = (fmi3Clock*)values;

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

 FMIStatus FMIHexToBinary(const char* hex, size_t size, unsigned char* binary) {

     if (hex == NULL || *hex == '\0') {
         FMILogError("Argument hex must not be NULL or an empty string.");
         return FMIError;
     }
        
     if (binary == NULL) {
         FMILogError("Argument binary must not be NULL.");
         return FMIError;
     }

     if (size != strlen(hex) / 2) {
         FMILogError("The length of a binary literal must be a multiple of two.");
         return FMIError;
     }

     for (size_t i = 0; i < size; i++) {
         char b1, b2;
         if (!hexchr2bin(hex[i * 2], &b1) || !hexchr2bin(hex[i * 2 + 1], &b2)) {
             FMILogError("\"%s\" is not a valid hex value.", hex);
             return FMIError;
         }
         binary[i] = (b1 << 4) | b2;
     }

     return FMIOK;
 }

FMIStatus FMIRestoreFMUStateFromFile(FMIInstance* S, const char* filename) {

     FMIStatus status = FMIOK;

     FILE* file = NULL;

     file = fopen(filename, "rb");

     if (!file) {
         // TODO: log message
         return FMIError;
     }

     fseek(file, 0L, SEEK_END);
     
     const size_t serializedFMUStateSize = ftell(file);

     fseek(file, 0L, SEEK_SET);

     char* serializedFMUState = NULL;

     CALL(FMICalloc((void**)&serializedFMUState, serializedFMUStateSize, sizeof(char)));

     fread(serializedFMUState, sizeof(char), serializedFMUStateSize, file);

     fclose(file);

     void* FMUState = NULL;

     switch (S->fmiMajorVersion) {
     case FMIMajorVersion2:
         CALL(FMI2DeSerializeFMUstate(S, serializedFMUState, serializedFMUStateSize, &FMUState));
         CALL(FMI2SetFMUstate(S, FMUState));
         CALL(FMI2FreeFMUstate(S, FMUState));
         break;
     case FMIMajorVersion3:
         CALL(FMI3DeserializeFMUState(S, serializedFMUState, serializedFMUStateSize, &FMUState));
         CALL(FMI3SetFMUState(S, FMUState));
         CALL(FMI3FreeFMUState(S, FMUState));
         break;
     default:
         // TODO: log message
         status = FMIError;
         break;
     }

 TERMINATE:

     free(serializedFMUState);

     return status;
}

FMIStatus FMISaveFMUStateToFile(FMIInstance* S, const char* filename) {

    if (S->fmiMajorVersion == FMIMajorVersion1) {
        return FMIError;
    }

    FMIStatus status = FMIOK;

    void* FMUState = NULL;

    if (S->fmiMajorVersion == FMIMajorVersion2) {
        CALL(FMI2GetFMUstate(S, &FMUState));
    } else {
        CALL(FMI3GetFMUState(S, &FMUState));
    }

    size_t serializedFMUStateSize = 0;

    if (S->fmiMajorVersion == FMIMajorVersion2) {
        CALL(FMI2SerializedFMUstateSize(S, FMUState, &serializedFMUStateSize));
    } else {
        CALL(FMI3SerializedFMUStateSize(S, FMUState, &serializedFMUStateSize));
    }

    char* serializedFMUState = NULL; 
    
    CALL(FMICalloc((void**)&serializedFMUState, serializedFMUStateSize, sizeof(char)));

    if (S->fmiMajorVersion == FMIMajorVersion2) {
        CALL(FMI2SerializeFMUstate(S, FMUState, serializedFMUState, serializedFMUStateSize));
    } else {
        CALL(FMI3SerializeFMUState(S, FMUState, serializedFMUState, serializedFMUStateSize));
    }

    FILE* file = fopen(filename, "wb");

    if (!file) {
        status = FMIError;
        goto TERMINATE;
    }

    const size_t written = fwrite(serializedFMUState, sizeof(char), serializedFMUStateSize, file);

    fclose(file);

    if (written != serializedFMUStateSize) {
        status = FMIError;
        goto TERMINATE;
    }

    free(serializedFMUState);

    if (S->fmiMajorVersion == FMIMajorVersion2) {
        CALL(FMI2FreeFMUstate(S, FMUState));
    } else {
        CALL(FMI3FreeFMUState(S, FMUState));
    }

TERMINATE:

    return status;
}

FMIStatus FMIDuplicateString(const char* source, char** destination) {

    if (!source || !destination) {
        return FMIError;
    }

    const size_t length = strlen(source);

    char* temp = NULL;

    if (FMICalloc((void**)&temp, length + 1, sizeof(char)) != FMIOK) {
        return FMIError;
    }

    memcpy(temp, source, length + 1);

    *destination = temp;

    return FMIOK;
}

FMIStatus FMIDuplicateBuffer(const void* source, void** destination, size_t size) {

    if (!destination) {
        FMILogError("Pointer to destination must not be NULL.");
        return FMIError;
    }

    if (size == 0) {
        *destination = NULL;
        return FMIOK;
    }

    if (!source) {
        FMILogError("Pointer to source must not be NULL.");
        return FMIError;
    }

    void* temp = NULL;

    if (FMICalloc(&temp, size, 1) != FMIOK) {
        return FMIError;
    }

    memcpy(temp, source, size);

    *destination = temp;

    return FMIOK;
}

#define EPSILON (1.0e-5)

bool FMIIsClose(double a, double b) {

    if (!isfinite(a) || !isfinite(b)) {
        return false;
    }

    if (fabs(a - b) <= EPSILON) {
        return true;
    }

    return fabs(a - b) <= EPSILON * fmax(fabs(a), fabs(b));
}
