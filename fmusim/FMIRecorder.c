#include <inttypes.h>
#include <stdlib.h>

#include "FMI1.h"
#include "FMI2.h"
#include "FMI3.h"
#include "FMIUtil.h"

#include "FMIRecorder.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIRecorder* FMICreateRecorder(size_t nVariables, const FMIModelVariable* variables[], const char* file) {

    FMIStatus status = FMIOK;

    FMIRecorder* result = NULL;
    
    CALL(FMICalloc(&result, 1, sizeof(FMIRecorder)));

    result->nVariables = nVariables;
    result->variables = variables;
    result->file = fopen(file, "w");

    if (!result->file) {
        free(result);
        return NULL;
    }

TERMINATE:

    if (status != FMIOK) {
        FMIFree(result);
    }

    return result;
}

void FMIFreeRecorder(FMIRecorder* result) {

    if (result) {
        
        if (result->file) {
            fclose(result->file);
        }

        free(result->values);
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
            fprintf(result->file, ",\"%s\"", result->variables[i]->name);
        }

        fputc('\n', result->file);

        result->instance = instance;
    }

    fprintf(file, "%.16g", time);

    for (size_t i = 0; i < result->nVariables; i++) {

        const FMIModelVariable* variable = result->variables[i];
        const FMIValueReference* vr = &variable->valueReference;
        const FMIVariableType type = variable->type;

        if (instance->fmiVersion == FMIVersion1) {

            if (type == FMIRealType || type == FMIDiscreteRealType) {
                fmi1Real value;
                CALL(FMI1GetReal(instance, vr, 1, &value));
                fprintf(file, ",%.16g", value);
            } else if (type == FMIIntegerType) {
                fmi1Integer value;
                CALL(FMI1GetInteger(instance, vr, 1, &value));
                fprintf(file, ",%d", value);
            } else if (type == FMIBooleanType) {
                fmi1Boolean value;
                CALL(FMI1GetBoolean(instance, vr, 1, &value));
                fprintf(file, ",%d", value);
            } else if (type == FMIStringType) {
                fmi1String value;
                CALL(FMI1GetString(instance, vr, 1, &value));
                fprintf(file, ",\"%s\"", value);
            }

        } else if (instance->fmiVersion == FMIVersion2) {

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

            size_t nValues;
            
            CALL(FMIGetNumberOfVariableValues(instance, variable, &nValues));

            if (result->nValues < nValues * 8) {

                result->nValues = nValues * 8;
                
                result->values = realloc(result->values, result->nValues);
                result->sizes = realloc(result->sizes, nValues * sizeof(size_t));
                
                if (!result->values || !result->sizes) {
                    FMILogError("Failed to allocate buffer.\n");
                    goto TERMINATE;
                }
            }

            fprintf(file, ",");

            if (type == FMIFloat32Type || type == FMIDiscreteFloat32Type) {

                fmi3Float32* values = (fmi3Float32*)result->values;

                CALL(FMI3GetFloat32(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%.7g", values[j]);
                }

            } else if (type == FMIFloat64Type || type == FMIDiscreteFloat64Type) {

                fmi3Float64* values = (fmi3Float64*)result->values;
                
                CALL(FMI3GetFloat64(instance, vr, 1, values, nValues));
                
                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%.16g", values[j]);
                }

            } else if (type == FMIInt8Type) {

                fmi3Int8* values = (fmi3Int8*)result->values;

                CALL(FMI3GetInt8(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRId8, values[j]);
                }

            } else if (type == FMIUInt8Type) {

                fmi3UInt8* values = (fmi3UInt8*)result->values;

                CALL(FMI3GetUInt8(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRIu8, values[j]);
                }

            } else if (type == FMIInt16Type) {

                fmi3Int16* values = (fmi3Int16*)result->values;

                CALL(FMI3GetInt16(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRId16, values[j]);
                }

            } else if (type == FMIUInt16Type) {

                fmi3UInt16* values = (fmi3UInt16*)result->values;

                CALL(FMI3GetUInt16(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRIu16, values[j]);
                }

            } else if (type == FMIInt32Type) {

                fmi3Int32* values = (fmi3Int32*)result->values;

                CALL(FMI3GetInt32(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRId32, values[j]);
                }

            } else if (type == FMIUInt32Type) {

                fmi3UInt32* values = (fmi3UInt32*)result->values;

                CALL(FMI3GetUInt32(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRIu32, values[j]);
                }

            } else if (type == FMIInt64Type) {

                fmi3Int64* values = (fmi3Int64*)result->values;

                CALL(FMI3GetInt64(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRId64, values[j]);
                }

            } else if (type == FMIUInt64Type) {

                fmi3UInt64* values = (fmi3UInt64*)result->values;

                CALL(FMI3GetUInt64(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRIu64, values[j]);
                }

            } else if (type == FMIBooleanType) {

                fmi3Boolean* values = (fmi3Boolean*)result->values;

                CALL(FMI3GetBoolean(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%d", values[j]);
                }

            } else if (type == FMIStringType) {

                fmi3String* values = (fmi3String*)result->values;

                CALL(FMI3GetString(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "\"%s\"", values[j]);
                }

            } else if (type == FMIBinaryType) {

                size_t* sizes = (size_t*)result->sizes;
                fmi3Binary* values = (fmi3String*)result->values;

                CALL(FMI3GetBinary(instance, vr, 1, sizes, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    
                    if (j > 0) fputc(' ', file);

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

                fmi3Clock value;

                CALL(FMI3GetClock(instance, vr, 1, &value));

                fprintf(file, ",%d", value);
                
            }

        }

    }

    fputc('\n', file);

TERMINATE:
    return status;
}
