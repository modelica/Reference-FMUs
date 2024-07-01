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

    // allocate memory
    CALL(FMICalloc(&recorder->float64Variables,       nVariables, sizeof(FMIModelVariable*)));
    CALL(FMICalloc(&recorder->float64Sizes,           nVariables, sizeof(size_t)));
    CALL(FMICalloc(&recorder->float64ValueReferences, nVariables, sizeof(FMIValueReference)));

    // collect variable infos
    for (size_t i = 0; i < nVariables; i++) {

        const FMIModelVariable* variable = variables[i];

        switch (variable->type) {
        case FMIFloat64Type:
        case FMIDiscreteFloat64Type:
            recorder->float64Variables[recorder->nFloat64Variables] = variable;
            recorder->float64Sizes[recorder->nFloat64Variables] = 1;
            recorder->float64ValueReferences[recorder->nFloat64Variables] = variable->valueReference;
            recorder->nFloat64Variables++;
            recorder->nFloat64Values++;
            break;
        default:
            break;
        }
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

    CALL(FMICalloc(&row->float64Values, recorder->nFloat64Values, sizeof(fmi3Float64)));

    recorder->rows[recorder->nRows] = row;

    row->time = time;

    if (recorder->nFloat64Variables) {
        CALL(FMI3GetFloat64(recorder->instance, recorder->float64ValueReferences, recorder->nFloat64Variables, row->float64Values, recorder->nFloat64Values));
    }

    recorder->nRows++;

TERMINATE:
    return status;
}

#define FLOAT64_FMT "%.16g"

FMIStatus FMIRecorderWriteCSV(FMIRecorder* recorder, FILE* file) {

    fprintf(file, "\"time\"");

    for (size_t i = 0; i < recorder->nFloat64Variables; i++) {
        
        FMIModelVariable* variable = recorder->float64Variables[i];
        
        fprintf(file, ",\"%s\"", variable->name);
    }

    fputc('\n', file);

    for (size_t i = 0; i < recorder->nRows; i++) {

        Row* row = recorder->rows[i];

        fprintf(file, FLOAT64_FMT, row->time);

        size_t l = 0;

        for (size_t j = 0; j < recorder->nFloat64Variables; j++) {

            fputc(',', file);

            const size_t nValues = recorder->float64Sizes[j];

            for (size_t k = 0; k < nValues; k++) {
                fprintf(file, FLOAT64_FMT, row->float64Values[l++]);
            }
        }

        fputc('\n', file);
    }

}
