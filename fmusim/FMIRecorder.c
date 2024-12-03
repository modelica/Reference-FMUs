#include <inttypes.h>
#include <stdlib.h>

#include "FMI1.h"
#include "FMI2.h"
#include "FMI3.h"
#include "FMIUtil.h"

#include "FMIRecorder.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIRecorder* FMICreateRecorder(FMIInstance* instance, size_t nVariables, const FMIModelVariable** variables) {

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
            CALL(FMICalloc((void**)&info, 1, sizeof(VariableInfo)));
            CALL(FMICalloc((void**)&info->variables, nVariables, sizeof(FMIModelVariable*)));
            CALL(FMICalloc((void**)&info->sizes, nVariables, sizeof(size_t)));
            CALL(FMICalloc((void**)&info->valueReferences, nVariables, sizeof(FMIValueReference)));
            recorder->variableInfos[variable->type] = info;
        }

        info->variables[info->nVariables] = (FMIModelVariable*)variable;
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

    if (!recorder) {
        return;
    }

    for (size_t i = 0; i < recorder->nRows; i++) {

        Row* row = recorder->rows[i];

        for (size_t j = 0; j < N_VARIABLE_TYPES; j++) {
            
            FMIVariableType type = (FMIVariableType)j;

            VariableInfo* info = recorder->variableInfos[j];
            void* values = row->values[j];

            if (!values) {
                continue;
            }

            if (type == FMIBinaryType || type == FMIStringType) {

                char** binaryValues = (char**)values;

                for (size_t k = 0; k < info->nValues; k++) {
                    FMIFree((void**)&binaryValues[k]);
                }
            }

            FMIFree(&values);
        }

        FMIFree((void**)&row->sizes);
    }

    for (size_t i = 0; i < N_VARIABLE_TYPES; i++) {

        VariableInfo* info = recorder->variableInfos[i];

        if (!info) {
            continue;
        }

        FMIFree((void**)&info->variables);
        FMIFree((void**)&info->sizes);
    }

    FMIFree((void**)&recorder);
}

FMIStatus FMIRecorderUpdateSizes(FMIRecorder* recorder) {

    FMIStatus status = FMIOK;

    if (!recorder) {
        goto TERMINATE;
    }

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

FMIStatus FMISample(FMIInstance* instance, double time, FMIRecorder* recorder) {

    FMIStatus status = FMIOK;

    if (!recorder) {
        goto TERMINATE;
    }

    CALL(FMIRealloc((void**)&recorder->rows, (recorder->nRows + 1) * sizeof(Row*)));

    Row* row = NULL;

    CALL(FMICalloc((void**)&row, 1, sizeof(Row)));

    row->time = time;

    recorder->rows[recorder->nRows] = row;

    for (size_t i = 0; i < N_VARIABLE_TYPES; i++) {

        const FMIVariableType type = (FMIVariableType)i;

        VariableInfo* info = recorder->variableInfos[i];

        if (!info) {
            continue;
        }

        if (type == FMIBinaryType) {
            CALL(FMICalloc((void**)&row->sizes, info->nValues, sizeof(size_t)));
        }

        void* values = NULL;

        CALL(FMICalloc(&values, info->nValues, FMISizeOfVariableType(type, recorder->instance->fmiMajorVersion)));

        if (type != FMIClockType) {  // skip clocks
            CALL(FMIGetValues(recorder->instance, type, info->valueReferences, info->nVariables, row->sizes, values, info->nValues));
        }

        if (type == FMIBinaryType) {

            for (size_t j = 0; j < info->nValues; j++) {
                CALL(FMIDuplicateBuffer(((void**)values)[j], &((void**)values)[j], row->sizes[j]));
            }

        } else if (type == FMIStringType) {

            for (size_t j = 0; j < info->nValues; j++) {
                CALL(FMIDuplicateString((char*)((void**)values)[j], (char**)&((void**)values)[j]));
            }

        }

        row->values[i] = values;
    }

    recorder->nRows++;

TERMINATE:
    return status;
}

static void print_value(FILE* file, FMIVariableType type, FMIMajorVersion fmiMajorVersion, void* value) {

    size_t size = 3;

    if (!value) {
        fprintf(file, "NULL");
        return;
    }

    switch (type) {
    case FMIFloat32Type:
        fprintf(file, "%.7g", *((fmi3Float32*)value));
        break;
    case FMIFloat64Type:
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
        switch (fmiMajorVersion) {
        case FMIMajorVersion1:
            fprintf(file, "%d", *((fmi1Boolean*)value));
            break;
        case FMIMajorVersion2:
            fprintf(file, "%d", *((fmi2Boolean*)value));
            break;
        case FMIMajorVersion3:
            fprintf(file, "%d", *((fmi3Boolean*)value));
            break;
        }
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

    const FMIMajorVersion fmiMajorVersion = recorder->instance->fmiMajorVersion;

    for (size_t i = 0; i < recorder->nRows; i++) {

        Row* row = recorder->rows[i];

        print_value(file, FMIFloat64Type, FMIMajorVersion3, &row->time);

        for (size_t j = 0; j < N_VARIABLE_TYPES; j++) {
            
            size_t m = 0;

            VariableInfo* info = recorder->variableInfos[j];

            if (!info) {
                continue;
            }

            const FMIVariableType type = (FMIVariableType)j;
            const size_t size = FMISizeOfVariableType(type, fmiMajorVersion);
            char* value = (char*)row->values[j];

            for (size_t k = 0; k < info->nVariables; k++) {

                fputc(',', file);

                for (size_t l = 0; l < info->sizes[k]; l++) {

                    if (l > 0) {
                        fputc(' ', file);
                    }
                    
                    print_value(file, type, fmiMajorVersion, value);
                    
                    value += size;
                }
            }
        }

        fputc('\n', file);
    }

    return FMIOK;
}
