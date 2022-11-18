#include <inttypes.h>
#include <stdlib.h>

#include "FMI2.h"
#include "FMI3.h"

#include "FMISimulationResult.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMISimulationResult* FMICreateSimulationResult(size_t nVariables, const FMIModelVariable* variables[], const char* file) {

    FMISimulationResult* result = calloc(1, sizeof(FMISimulationResult));

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

    fprintf(result->file, "\"time\"");

    for (size_t i = 0; i < nVariables; i++) {
        fprintf(result->file, ",\"%s\"", variables[i]->name);
    }

    fputc('\n', result->file);

    return result;
}

void FMIFreeSimulationResult(FMISimulationResult* result) {

    if (result) {
        
        if (result->file) {
            fclose(result->file);
        }

        free(result);
    }
}

FMIStatus FMISample(FMIInstance* instance, double time, FMISimulationResult* result) {

    FMIStatus status = FMIOK;

    if (!result) {
        goto TERMINATE;
    }

    FILE* file = result->file;

    if (!file) {
        goto TERMINATE;
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

            if (type == FMIFloat32Type || type == FMIDiscreteFloat32Type) {

                fmi3Float32 value;
                CALL(FMI3GetFloat32(instance, vr, 1, &value, 1));
                fprintf(file, ",%.7g", value);

            } else if (type == FMIFloat64Type || type == FMIDiscreteFloat64Type) {

                fmi3Float64 value;
                CALL(FMI3GetFloat64(instance, vr, 1, &value, 1));
                fprintf(file, ",%.16g", value);

            } else if (type == FMIInt8Type) {

                fmi3Int8 value;
                CALL(FMI3GetInt8(instance, vr, 1, &value, 1));
                fprintf(file, ",%" PRId8, value);

            } else if (type == FMIUInt8Type) {

                fmi3UInt8 value;
                CALL(FMI3GetUInt8(instance, vr, 1, &value, 1));
                fprintf(file, ",%" PRIu8, value);

            } else if (type == FMIInt16Type) {

                fmi3Int16 value;
                CALL(FMI3GetInt16(instance, vr, 1, &value, 1));
                fprintf(file, ",%" PRId16, value);

            } else if (type == FMIUInt16Type) {

                fmi3UInt16 value;
                CALL(FMI3GetUInt16(instance, vr, 1, &value, 1));
                fprintf(file, ",%" PRIu16, value);

            } else if (type == FMIInt32Type) {

                fmi3Int32 value;
                CALL(FMI3GetInt32(instance, vr, 1, &value, 1));
                fprintf(file, ",%" PRId32, value);

            } else if (type == FMIUInt32Type) {

                fmi3UInt32 value;
                CALL(FMI3GetUInt32(instance, vr, 1, &value, 1));
                fprintf(file, ",%" PRIu8, value);

            } else if (type == FMIInt64Type) {

                fmi3Int64 value;
                CALL(FMI3GetInt64(instance, vr, 1, &value, 1));
                fprintf(file, ",%" PRId64, value);

            } else if (type == FMIUInt64Type) {

                fmi3UInt64 value;
                CALL(FMI3GetUInt64(instance, vr, 1, &value, 1));
                fprintf(file, ",%" PRIu64, value);

            } else if (type == FMIBooleanType) {

                fmi3Boolean value;
                CALL(FMI3GetBoolean(instance, vr, 1, &value, 1));
                fprintf(file, ",%d", value);

            } else if (type == FMIStringType) {

                fmi3String value;
                CALL(FMI3GetString(instance, vr, 1, &value, 1));
                fprintf(file, ",\"%s\"", value);

            } else if (type == FMIBinaryType) {

                size_t size;
                char* value;

                CALL(FMI3GetBinary(instance, vr, 1, &size, &value, 1));

                fputc(',', file);

                for (size_t j = 0; j < size; j++) {
                    const char hex[3] = {
                        "0123456789abcdef"[value[j] >> 4],
                        "0123456789abcdef"[value[j] & 0x0F],
                        '\0'
                    };
                    fputs(hex, file);
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
