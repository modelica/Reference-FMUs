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
     case FMIDiscreteRealType:
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
    case FMIDiscreteRealType:
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
    case FMIDiscreteFloat32Type:
        return FMI3SetFloat32(instance, valueReferences, nValueReferences, (fmi3Float32*)values, nValues);
    case FMIFloat64Type:
    case FMIDiscreteFloat64Type:
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
        return FMIError;
    case FMIClockType:
        return FMI3SetClock(instance, valueReferences, nValueReferences, (fmi3Clock*)values);
    default:
        return FMIError;
    }
 }

FMIStatus FMICalloc(void** memory, size_t count, size_t size) {

    if (!memory) {
        printf("Pointer to memory must not be NULL.");
        return FMIError;
    }

    *memory = calloc(count, size);

    if (!*memory) {
        printf("Failed to reallocate memory.");
        return FMIError;
    }

    return FMIOK;
}

FMIStatus FMIRealloc(void** memory, size_t size) {

    if (!memory) {
        printf("Pointer to memory must not be NULL.");
        return FMIError;
    }

    void* temp = realloc(*memory, size);

    if (!temp) {
        printf("Failed to reallocate memory.");
        return FMIError;
    }

    *memory = temp;

    return FMIOK;
}

#define PARSE_VALUES(t, f, ...) \
    while (strlen(next) > 0) { \
        CALL(FMIRealloc(values, sizeof(t)* ((*nValues) + 1))); \
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

FMIStatus FMIParseValues(FMIVersion fmiVersion, FMIVariableType type, const char* literal, size_t* nValues, void** values) {

    FMIStatus status = FMIOK;

    if (!literal) {
        printf("Value literal must not be NULL.\n");
        return FMIError;
    }

    *nValues = 0;
    *values = NULL;

    char* next = (char*)literal;

    switch (type) {
    case FMIFloat32Type:
    case FMIDiscreteFloat32Type:
        PARSE_VALUES(fmi3Float32, strtof);
        break;
    case FMIFloat64Type:
    case FMIDiscreteFloat64Type:
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
        CALL(FMIRealloc(values, sizeof(fmi3String)));
        fmi3String* v = *values;
        v[0] = literal;
        *nValues = 1;
        break;
    }
    case FMIBooleanType:
    case FMIClockType: {

        size_t size = 0;

        switch (fmiVersion) {
        case FMIVersion1:
            size = sizeof(fmi1Boolean);
            break;
        case FMIVersion2:
            size = sizeof(fmi2Boolean);
            break;
        case FMIVersion3:
            size = sizeof(fmi2Boolean);
            break;
        }

        while (strlen(next) > 0) {

            CALL(FMIRealloc(values, size * ((*nValues) + 1)));
            
            fmi3Boolean* v = (fmi3Boolean*)*values;

            size_t delimiter = strcspn(next, " ");

            if (!strncmp(next, "0", delimiter) || !strncmp(next, "false", delimiter)) {
                v[*nValues] = fmi3False;
            } else if (!strncmp(next, "1", delimiter) || !strncmp(next, "true", delimiter)) {
                v[*nValues] = fmi3True;
            } else {
                printf("Values for boolean must be one of 0, false, 1, or true.\n");
                status = FMIError;
                goto TERMINATE;
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
        const size_t size = strlen(literal) / 2;
        CALL(FMICalloc(values, 1, sizeof(fmi3Binary) + size * sizeof(fmi3Byte)));
        fmi3Binary* v = (fmi3Binary*)*values;
        fmi3Byte* b = (fmi3Byte*)&v[1];
        CALL(FMIHexToBinary(literal, size, b));
        v[0] = b;
        *nValues = 1;
        break;
    }
    default:
        printf("Unsupported value type.");
        status = FMIError;
        goto TERMINATE;
    }

TERMINATE:
    if (status != FMIOK) {
        *nValues = 0;
        free(*values);
        *values = NULL;
        printf("Failed to parse value literal \"%s\".\n", literal);
    }

    return status;
}

 FMIStatus FMIParseStartValues(FMIVariableType type, const char* literal, size_t nValues, void* values) {

     char* next = (char*)literal;

     if (type == FMIFloat32Type || type == FMIDiscreteFloat32Type) {

         fmi3Float32* v = (fmi3Float32*)values;

         for (size_t i = 0; i < nValues; i++) {
             v[i] = strtof(next, &next);
         }

     } else if (type == FMIFloat64Type || type == FMIDiscreteFloat64Type) {

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

     if (hex == NULL || *hex == '\0' || binary == NULL) {
         // TODO: print message
         return FMIError;
     }
         
     if (size != strlen(hex) / 2) {
         // TODO: print message
         return FMIError;
     }

     for (size_t i = 0; i < size; i++) {
         char b1, b2;
         if (!hexchr2bin(hex[i * 2], &b1) || !hexchr2bin(hex[i * 2 + 1], &b2)) {
             // TODO: print message
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
         return FMIError;
     }

     fseek(file, 0L, SEEK_END);
     
     const size_t serializedFMUStateSize = ftell(file);

     fseek(file, 0L, SEEK_SET);

     char* serializedFMUState = (char*)calloc(serializedFMUStateSize, sizeof(char));

     if (!serializedFMUState) {
         return FMIError;
     }

     fread(serializedFMUState, sizeof(char), serializedFMUStateSize, file);

     fclose(file);

     void* FMUState = NULL;

     switch (S->fmiVersion) {
     case FMIVersion2:
         CALL(FMI2DeSerializeFMUstate(S, serializedFMUState, serializedFMUStateSize, &FMUState));
         CALL(FMI2SetFMUstate(S, FMUState));
         CALL(FMI2FreeFMUstate(S, FMUState));
         break;
     case FMIVersion3:
         CALL(FMI3DeserializeFMUState(S, serializedFMUState, serializedFMUStateSize, &FMUState));
         CALL(FMI3SetFMUState(S, FMUState));
         CALL(FMI3FreeFMUState(S, FMUState));
         break;
     default:
         status = FMIError;
         break;
     }

 TERMINATE:

     free(serializedFMUState);

     return status;
}

FMIStatus FMISaveFMUStateToFile(FMIInstance* S, const char* filename) {

    if (S->fmiVersion == FMIVersion1) {
        return FMIError;
    }

    FMIStatus status = FMIOK;

    void* FMUState = NULL;

    if (S->fmiVersion == FMIVersion2) {
        CALL(FMI2GetFMUstate(S, &FMUState));
    } else {
        CALL(FMI3GetFMUState(S, &FMUState));
    }

    size_t serializedFMUStateSize = 0;

    if (S->fmiVersion == FMIVersion2) {
        CALL(FMI2SerializedFMUstateSize(S, FMUState, &serializedFMUStateSize));
    } else {
        CALL(FMI3SerializedFMUStateSize(S, FMUState, &serializedFMUStateSize));
    }

    char* serializedFMUState = (char*)calloc(serializedFMUStateSize, sizeof(char));

    if (!serializedFMUState) {
        status = FMIError;
        goto TERMINATE;
    }

    if (S->fmiVersion == FMIVersion2) {
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

    if (S->fmiVersion == FMIVersion2) {
        CALL(FMI2FreeFMUstate(S, FMUState));
    } else {
        CALL(FMI3FreeFMUState(S, FMUState));
    }

TERMINATE:

    return status;
}
