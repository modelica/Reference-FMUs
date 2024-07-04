#include <inttypes.h>
#include <stdlib.h>

#include "FMI1.h"
#include "FMI2.h"
#include "FMI3.h"
#include "FMIUtil.h"

#include "FMIRecorder.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIRecorder* FMICreateRecorder(FMIInstance* instance, size_t nVariables, const FMIModelVariable** variables, const char* file) {

    FMIStatus status = FMIOK;

    FMIRecorder* recorder = NULL;
    
    CALL(FMICalloc((void**)&recorder, 1, sizeof(FMIRecorder)));

    recorder->nVariables = nVariables;

    CALL(FMICalloc((void**)&recorder->variables, nVariables, sizeof(FMIModelVariable*)));
    memcpy(recorder->variables, variables, nVariables * sizeof(FMIModelVariable*));

    recorder->file = fopen(file, "w");

    if (!recorder->file) {
        status = FMIError;
        goto TERMINATE;
    }

    // DATA
    recorder->instance   = instance;

    // collect variable infos
    for (size_t i = 0; i < nVariables; i++) {

        const FMIModelVariable* variable = variables[i];

        VariableInfo* info = recorder->variableInfos[variable->type];

        if (!info) {
            CALL(FMICalloc(&info, 1, sizeof(VariableInfo)));
            CALL(FMICalloc(&info->variables, nVariables, sizeof(FMIModelVariable*)));
            CALL(FMICalloc(&info->sizes, nVariables, sizeof(size_t)));
            CALL(FMICalloc(&info->valueReferences, nVariables, sizeof(FMIValueReference)));
            recorder->variableInfos[variable->type] = info;
        }

        info->variables[info->nVariables] = variable;
        info->sizes[info->nVariables] = 1;
        info->valueReferences[info->nVariables] = variable->valueReference;
        info->nValues++;
        info->nVariables++;
    }

    recorder->nRows = 0;
    recorder->rows  = NULL;

TERMINATE:

    if (status != FMIOK) {
        FMIFree((void**)&recorder);
    }

    return recorder;
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

FMIStatus FMIRecorderUpdateSizes(FMIRecorder* recorder) {

    FMIStatus status = FMIOK;

    for (size_t i = 0; i < N_VARIABLE_TYPES; i++) {

        VariableInfo* info = recorder->variableInfos[i];

        if (!info) {
            continue;
        }

        info->nValues = 0;

        for (size_t j = 0; j < info->nVariables; j++) {
            FMIModelVariable* variable = info->variables[j];
            size_t nValues;
            CALL(FMIGetNumberOfVariableValues(recorder->instance, variable, &nValues));
            info->sizes[j] = nValues;
            info->nValues += nValues;
        }
    }

TERMINATE:
    
    return status;
}

static size_t FMISizeOfVariableType(FMIMajorVersion majorVersion, FMIVariableType type) {
    
    switch (type) {
    case FMIFloat32Type:
    case FMIDiscreteFloat32Type:
        return sizeof(fmi3Float32);

    case FMIFloat64Type:
    case FMIDiscreteFloat64Type:
        return sizeof(fmi3Float64);

    case FMIInt8Type:
        return sizeof(fmi3Int8);

    case FMIUInt8Type:
        return sizeof(fmi3UInt8);

    case FMIInt16Type:
        return sizeof(fmi3Int16);

    case FMIUInt16Type:
        return sizeof(fmi3UInt16);

    case FMIInt32Type:
        return sizeof(fmi3Int32);

    case FMIUInt32Type:
        return sizeof(fmi3UInt32);

    case FMIInt64Type:
        return sizeof(fmi3Int64);

    case FMIUInt64Type:
        return sizeof(fmi3UInt64);

    case FMIBooleanType:
        switch (majorVersion) {
        case FMIMajorVersion1:
            return sizeof(fmi1Boolean);
        case FMIMajorVersion2:
            return sizeof(fmi2Boolean);
        case FMIMajorVersion3:
            return sizeof(fmi3Boolean);
        }

    case FMIStringType:
        return sizeof(fmi3String);

    case FMIBinaryType:
        return sizeof(fmi3Binary);

    case FMIClockType:
        return sizeof(fmi3Clock);

    case FMIValueReferenceType:
        return sizeof(FMIValueReference);

    case FMISizeTType:
        return sizeof(size_t);

    default:
        return 0;
    }
}

static FMIStatus FMI3GetValues(
    FMIInstance* instance,
    FMIVariableType type,
    const FMIValueReference valueReferences[],
    size_t nValueReferences,
    size_t sizes[],
    void* values,
    size_t nValues) {

    switch (type) {
    case FMIFloat32Type:
    case FMIDiscreteFloat32Type:
        return FMI3GetFloat32(instance, valueReferences, nValueReferences, (fmi3Float32*)values, nValues);
    case FMIFloat64Type:
    case FMIDiscreteFloat64Type:
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

}

//FMIStatus FMICopyBuffer(const void* source, void** destination, size_t size) {
//
//    char* buffer = NULL;
//    
//    if (FMICalloc(&buffer, size, sizeof(fmi3Byte)) != FMIOK) {
//        return NULL;
//    }
//    
//    if (!memcpy(buffer, source, size)) {
//        FMIFree(&buffer);
//        return FMIError;
//    }
//    
//    *destination = buffer;
//
//    return FMIOK;
//}

FMIStatus FMISample(FMIInstance* instance, double time, FMIRecorder* recorder) {

    FMIStatus status = FMIOK;

    if (!recorder) {
        goto TERMINATE;
    }

    FILE* file = recorder->file;

    if (!file) {
        goto TERMINATE;
    }

    if (!recorder->instance) {

        fprintf(recorder->file, "\"time\"");

        for (size_t i = 0; i < recorder->nVariables; i++) {
            const FMIModelVariable* variable = recorder->variables[i];
            fprintf(recorder->file, ",\"%s\"", recorder->variables[i]->name);
        }

        fputc('\n', recorder->file);

        recorder->instance = instance;
    }

    fprintf(file, "%.16g", time);

    for (size_t i = 0; i < recorder->nVariables; i++) {

        continue;

        const FMIModelVariable* variable = recorder->variables[i];
        const FMIValueReference* vr = &variable->valueReference;
        const FMIVariableType type = variable->type;

        if (instance->fmiMajorVersion == FMIMajorVersion1) {

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

        } else if (instance->fmiMajorVersion == FMIMajorVersion2) {

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

        } else if (instance->fmiMajorVersion == FMIMajorVersion3) {

            size_t nValues;
            
            CALL(FMIGetNumberOfVariableValues(instance, variable, &nValues));

            if (recorder->nValues < nValues * 8) {

                recorder->nValues = nValues * 8;
                
                recorder->values = realloc(recorder->values, recorder->nValues);
                recorder->sizes = realloc(recorder->sizes, nValues * sizeof(size_t));
                
                if (!recorder->values || !recorder->sizes) {
                    FMILogError("Failed to allocate buffer.\n");
                    goto TERMINATE;
                }
            }

            fprintf(file, ",");

            if (type == FMIFloat32Type || type == FMIDiscreteFloat32Type) {

                fmi3Float32* values = (fmi3Float32*)recorder->values;

                CALL(FMI3GetFloat32(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%.7g", values[j]);
                }

            } else if (type == FMIFloat64Type || type == FMIDiscreteFloat64Type) {

                fmi3Float64* values = (fmi3Float64*)recorder->values;
                
                CALL(FMI3GetFloat64(instance, vr, 1, values, nValues));
                
                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%.16g", values[j]);
                }

            } else if (type == FMIInt8Type) {

                fmi3Int8* values = (fmi3Int8*)recorder->values;

                CALL(FMI3GetInt8(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRId8, values[j]);
                }

            } else if (type == FMIUInt8Type) {

                fmi3UInt8* values = (fmi3UInt8*)recorder->values;

                CALL(FMI3GetUInt8(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRIu8, values[j]);
                }

            } else if (type == FMIInt16Type) {

                fmi3Int16* values = (fmi3Int16*)recorder->values;

                CALL(FMI3GetInt16(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRId16, values[j]);
                }

            } else if (type == FMIUInt16Type) {

                fmi3UInt16* values = (fmi3UInt16*)recorder->values;

                CALL(FMI3GetUInt16(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRIu16, values[j]);
                }

            } else if (type == FMIInt32Type) {

                fmi3Int32* values = (fmi3Int32*)recorder->values;

                CALL(FMI3GetInt32(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRId32, values[j]);
                }

            } else if (type == FMIUInt32Type) {

                fmi3UInt32* values = (fmi3UInt32*)recorder->values;

                CALL(FMI3GetUInt32(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRIu32, values[j]);
                }

            } else if (type == FMIInt64Type) {

                fmi3Int64* values = (fmi3Int64*)recorder->values;

                CALL(FMI3GetInt64(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRId64, values[j]);
                }

            } else if (type == FMIUInt64Type) {

                fmi3UInt64* values = (fmi3UInt64*)recorder->values;

                CALL(FMI3GetUInt64(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%" PRIu64, values[j]);
                }

            } else if (type == FMIBooleanType) {

                fmi3Boolean* values = (fmi3Boolean*)recorder->values;

                CALL(FMI3GetBoolean(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "%d", values[j]);
                }

            } else if (type == FMIStringType) {

                fmi3String* values = (fmi3String*)recorder->values;

                CALL(FMI3GetString(instance, vr, 1, values, nValues));

                for (size_t j = 0; j < nValues; j++) {
                    if (j > 0) fputc(' ', file);
                    fprintf(file, "\"%s\"", values[j]);
                }

            } else if (type == FMIBinaryType) {

                size_t* sizes = (size_t*)recorder->sizes;
                fmi3Binary* values = (fmi3Binary*)recorder->values;

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

    // DATA
    CALL(FMIRealloc(&recorder->rows, (recorder->nRows + 1) * sizeof(Row*)));

    Row* row = NULL;

    CALL(FMICalloc(&row, 1, sizeof(Row)));

    row->time = time;

    recorder->rows[recorder->nRows] = row;

    for (size_t i = 0; i < N_VARIABLE_TYPES; i++) {

        const FMIVariableType type = (FMIVariableType)i;

        VariableInfo* info = recorder->variableInfos[i];

        if (!info) {
            continue;
        }

        if (type == FMIBinaryType) {
            CALL(FMICalloc(&row->sizes, info->nValues, sizeof(size_t)));
        }

        if (
            i != FMIFloat32Type &&
            i != FMIDiscreteFloat32Type &&
            i != FMIFloat64Type &&
            i != FMIDiscreteFloat64Type &&
            i != FMIInt8Type &&
            i != FMIUInt8Type &&
            i != FMIInt16Type &&
            i != FMIUInt16Type &&
            i != FMIInt32Type &&
            i != FMIUInt32Type &&
            i != FMIInt64Type &&
            i != FMIUInt64Type &&
            i != FMIBooleanType &&
            i != FMIStringType &&
            i != FMIBinaryType
            ) {
            continue;
        }

        void* values = NULL;

        CALL(FMICalloc(&values, info->nValues, FMISizeOfVariableType(FMIMajorVersion3, type)));

        CALL(FMI3GetValues(recorder->instance, type, info->valueReferences, info->nVariables, row->sizes, values, info->nValues));

        if (type == FMIBinaryType) {

            for (size_t j = 0; j < info->nValues; j++) {
                CALL(FMIDuplicateBuffer(((void**)values)[j], &((void**)values)[j], row->sizes[j]));
            }

        } else if (type == FMIStringType) {

            for (size_t j = 0; j < info->nValues; j++) {
                CALL(FMIDuplicateString( ((void**)values)[j], &((void**)values)[j]));
            }

        }

        row->values[i] = values;
    }

    recorder->nRows++;

TERMINATE:
    return status;
}

static void print_value(FILE* file, FMIVariableType type, void* value) {

    size_t size = 3;

    if (!value) {
        fprintf(file, "NULL");
        return;
    }

    switch (type) {
    case FMIFloat32Type:
    case FMIDiscreteFloat32Type:
        fprintf(file, "%.7g", *((fmi3Float32*)value));
        break;
    case FMIFloat64Type:
    case FMIDiscreteFloat64Type:
        fprintf(file, "%.16g", *((fmi3Float64*)value));
        break;
    case FMIInt8Type:
        fprintf(file, "%" PRId8, *((fmi3Int8*)value));
        break;
    case FMIUInt8Type:
        fprintf(file, "%" PRIu8, *((fmi3UInt8*)value));
        break;
    case FMIInt16Type:
        fprintf(file, "%" PRId16, *((fmi3Int16*)value));
        break;
    case FMIUInt16Type:
        fprintf(file, "%" PRIu16, *((fmi3UInt16*)value));
        break;
    case FMIInt32Type:
        fprintf(file, "%" PRId32, *((fmi3Int32*)value));
        break;
    case FMIUInt32Type:
        fprintf(file, "%" PRIu32, *((fmi3UInt32*)value));
        break;
    case FMIInt64Type:
        fprintf(file, "%" PRId64, *((fmi3Int64*)value));
        break;
    case FMIUInt64Type:
        fprintf(file, "%" PRIu64, *((fmi3UInt64*)value));
        break;
    case FMIBooleanType:
        fprintf(file, "%d", *((fmi3Boolean*)value));
        break;
    case FMIStringType:
        fprintf(file, "\"%s\"", *((fmi3String*)value));
        break;
    case FMIBinaryType: {

            const fmi3Binary v = *((fmi3Binary*)value);

            for (size_t i = 0; i < size; i++) {

                const fmi3Byte b = v[i];

                const char hex[3] = {
                    "0123456789abcdef"[b >> 4],
                    "0123456789abcdef"[b & 0x0F],
                    '\0'
                };

                fputs(hex, file);
            }
        }
        break;
    default:
        fprintf(file, "x");
        break;
    }
}


FMIStatus FMIRecorderWriteCSV(FMIRecorder* recorder, FILE* file) {

    fprintf(file, "\"time\"");

    for (size_t i = 0; i < N_VARIABLE_TYPES; i++) {

        VariableInfo* info = recorder->variableInfos[i];

        if (!info) {
            continue;
        }

        for (size_t j = 0; j < info->nVariables; j++) {

            FMIModelVariable* variable = info->variables[j];

            fprintf(file, ",\"%s\"", variable->name);
        }
    }

    fputc('\n', file);

    for (size_t i = 0; i < recorder->nRows; i++) {

        Row* row = recorder->rows[i];

        print_value(file, FMIFloat64Type, &row->time);

        for (size_t j = 0; j < N_VARIABLE_TYPES; j++) {
            
            size_t m = 0;

            VariableInfo* info = recorder->variableInfos[j];

            if (!info) {
                continue;
            }

            const FMIVariableType type = (FMIVariableType)j;
            const size_t size = FMISizeOfVariableType(FMIMajorVersion3, type);
            char* value = (char*)row->values[j];

            for (size_t k = 0; k < info->nVariables; k++) {

                fputc(',', file);

                //if (type != FMIFloat64Type && type != FMIInt32Type && type != FMIInt64Type) {
                //    fprintf(file, "?");
                //    continue;
                //}

                for (size_t l = 0; l < info->sizes[k]; l++) {

                    if (l > 0) {
                        fputc(' ', file);
                    }
                    
                    print_value(file, type, value);
                    
                    value += size;
                }
            }
        }

        fputc('\n', file);
    }

}
