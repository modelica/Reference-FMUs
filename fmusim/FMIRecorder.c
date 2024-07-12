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
    CALL(FMICalloc((void**)&recorder->variables, nVariables, sizeof(FMIModelVariable*)));
    CALL(FMICalloc((void**)&recorder->valueReferences, nVariables, sizeof(FMIValueReference*)));

    recorder->instance = instance;
    recorder->nVariables = nVariables;

    // collect variable infos
    for (size_t i = 0; i < nVariables; i++) {

        const FMIModelVariable* variable = variables[i];

        recorder->variables[i] = variable;
        recorder->valueReferences[i] = variable->valueReference;

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
    recorder->rows = NULL;

TERMINATE:

    if (status != FMIOK) {
        FMIFree((void**)&recorder);
    }

    return recorder;
}

void FMIFreeRecorder(FMIRecorder* recorder) {

    if (recorder) {

        free(recorder->values);
        free(recorder->sizes);

        free(recorder);
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

FMIStatus FMISample(FMIInstance* instance, double time, FMIRecorder* recorder) {

    FMIStatus status = FMIOK;

    if (!recorder) {
        goto TERMINATE;
    }

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

        void* values = NULL;

        CALL(FMICalloc(&values, info->nValues, FMISizeOfVariableType(recorder->instance->fmiMajorVersion, type)));

        CALL(FMIGetValues(recorder->instance, type, info->valueReferences, info->nVariables, row->sizes, values, info->nValues));

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
        fprintf(file, "?");
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
