#include <inttypes.h>
#include <stdlib.h>

#include "FMI2.h"
#include "FMI3.h"

#include "FMIRecorder.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIRecorder* FMICreateRecorder(size_t nVariables, const FMIModelVariable* variables[], const char* file) {

    FMIRecorder* result = calloc(1, sizeof(FMIRecorder));

    if (!result) {
        return NULL;
    }

    result->nVariables = nVariables;
    result->variables = variables;
    result->file = fopen(file, "w");

    if (!result->file) {
        free(result);
        return NULL;
    }

    return result;
}

void FMIFreeRecorder(FMIRecorder* result) {

    if (result) {
        
        if (result->file) {
            fclose(result->file);
        }

        free(result->buffer);
        free(result->sizes);

        free(result);
    }
}

FMIStatus FMISample(FMIInstance* instance, double time, FMIRecorder* result) {

    FMIStatus status = FMIOK;

    if (!result) {
        goto TERMINATE;
    }

    FILE* file = result->file;

    if (!file) {
        goto TERMINATE;
    }

    if (!result->instance) {

        fprintf(result->file, "\"time\"");

        for (size_t i = 0; i < result->nVariables; i++) {

            const FMIModelVariable* variable = result->variables[i];

            const char* name = result->variables[i]->name;

            if (variable->nDimensions == 0) {
                fprintf(result->file, ",\"%s\"", name);
            } else {
                const size_t nValues = FMIGetNumberOfVariableValues(instance, variable);
                for (size_t j = 0; j < nValues; j++) {
                    fprintf(result->file, ",\"%s[%zu]\"", name, j);
                }
            }

        }

        fputc('\n', result->file);

        result->instance = instance;
    }

    fprintf(file, "%.16g", time);

    for (size_t i = 0; i < result->nVariables; i++) {

        const FMIModelVariable* variable = result->variables[i];
        const FMIValueReference* vr = &variable->valueReference;
        const FMIVariableType type = variable->type;

        if (instance->fmiVersion == FMIVersion2) {

            if (type == FMIRealType || type == FMIDiscreteRealType) {
                fmi2Real value;
                CALL(FMI2GetReal(instance, vr, 1, &value));
                fprintf(file, ",%.16g", value);
            } else if (type == FMIIntegerType) {
                fmi2Integer value;
                CALL(FMI2GetInteger(instance, vr, 1, &value));
                fprintf(file, ",%d", value);
            } else if (type == FMIBooleanType) {
                fmi2Boolean value;
                CALL(FMI2GetBoolean(instance, vr, 1, &value));
                fprintf(file, ",%d", value);
            } else if (type == FMIStringType) {
                fmi2String value;
                CALL(FMI2GetString(instance, vr, 1, &value));
                fprintf(file, ",\"%s\"", value);
            }

        } else if (instance->fmiVersion == FMIVersion3) {

            const size_t nValues = FMIGetNumberOfVariableValues(instance, variable);

            if (result->bufferSize < nValues * 8) {

                result->bufferSize = nValues * 8;
                
                result->buffer = realloc(result->buffer, result->bufferSize);
                result->sizes = realloc(result->sizes, nValues * sizeof(size_t));
                
                if (!result->buffer || !result->sizes) {
                    printf("Failed to allocate buffer.\n");
                    goto TERMINATE;
                }
            }

            if (type == FMIFloat32Type || type == FMIDiscreteFloat32Type) {

                fmi3Float32* values = (fmi3Float32*)result->buffer;

                CALL(FMI3GetFloat32(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%.7g", values[j]);
                }

            } else if (type == FMIFloat64Type || type == FMIDiscreteFloat64Type) {

                fmi3Float64* values = (fmi3Float64*)result->buffer;
                
                CALL(FMI3GetFloat64(instance, vr, 1, values, nValues));
                
                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%.16g", values[j]);
                }

            } else if (type == FMIInt8Type) {

                fmi3Int8* value = (fmi3Int8*)result->buffer;

                CALL(FMI3GetInt8(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%" PRId8, value[j]);
                }

            } else if (type == FMIUInt8Type) {

                fmi3UInt8* value = (fmi3UInt8*)result->buffer;

                CALL(FMI3GetUInt8(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%" PRIu8, value[j]);
                }

            } else if (type == FMIInt16Type) {

                fmi3Int16* value = (fmi3Int16*)result->buffer;

                CALL(FMI3GetInt16(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%" PRId16, value[j]);
                }

            } else if (type == FMIUInt16Type) {

                fmi3UInt16* value = (fmi3UInt16*)result->buffer;

                CALL(FMI3GetUInt16(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%" PRIu16, value[j]);
                }

            } else if (type == FMIInt32Type) {

                fmi3Int32* value = (fmi3Int32*)result->buffer;

                CALL(FMI3GetInt32(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%" PRId32, value[j]);
                }

            } else if (type == FMIUInt32Type) {

                fmi3UInt32* value = (fmi3UInt32*)result->buffer;

                CALL(FMI3GetUInt32(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%" PRIu32, value[j]);
                }

            } else if (type == FMIInt64Type) {

                fmi3Int64* value = (fmi3Int64*)result->buffer;

                CALL(FMI3GetInt64(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%" PRId64, value[j]);
                }

            } else if (type == FMIUInt64Type) {

                fmi3UInt64* value = (fmi3UInt64*)result->buffer;

                CALL(FMI3GetUInt64(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%" PRIu64, value[j]);
                }

            } else if (type == FMIBooleanType) {

                fmi3Boolean* value = (fmi3Boolean*)result->buffer;

                CALL(FMI3GetBoolean(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%d", value[j]);
                }

            } else if (type == FMIStringType) {

                fmi3String* value = (fmi3String*)result->buffer;

                CALL(FMI3GetString(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",\"%s\"", value[j]);
                }

            } else if (type == FMIBinaryType) {

                size_t* sizes = (size_t*)result->sizes;
                fmi3Binary* values = (fmi3String*)result->buffer;

                CALL(FMI3GetBinary(instance, vr, 1, sizes, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fputc(',', file);

                    for (size_t k = 0; k < sizes[j]; k++) {
                        const char* value = values[j];
                        const char hex[3] = {
                            "0123456789abcdef"[value[k] >> 4],
                            "0123456789abcdef"[value[k] & 0x0F],
                            '\0'
                        };
                        fputs(hex, file);
                    }
                }

            } else if (type == FMIClockType) {

                fmi3Clock* value = (fmi3Boolean*)result->buffer;

                CALL(FMI3GetClock(instance, vr, 1, value, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    fprintf(file, ",%d", value[j]);
                }

            }

        }

    }

    fputc('\n', file);

TERMINATE:
    return status;
}

size_t FMIGetNumberOfVariableValues(FMIInstance* instance, const FMIModelVariable* variable) {
    
    FMIStatus status = FMIOK;
 
    size_t nValues = 1;

    if (variable->nDimensions > 0) {

        for (size_t j = 0; j < variable->nDimensions; j++) {

            const FMIDimension* dimension = &variable->dimensions[j];

            fmi3UInt64 extent;

            if (dimension->variable) {
                CALL(FMI3GetUInt64(instance, &dimension->variable->valueReference, 1, &extent, 1));
            } else {
                extent = dimension->start;
            }

            nValues *= extent;
        }

    }

TERMINATE:
    if (status > FMIOK) {
        return 0;
    }

    return nValues;
}
