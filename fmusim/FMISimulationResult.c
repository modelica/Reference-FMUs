#include <inttypes.h>
#include "FMI2.h"
#include "FMI3.h"
#include "FMISimulationResult.h"
#include <stdlib.h>


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


size_t FMISizeForVariableType(FMIVariableType type) {

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
            return sizeof(fmi3Boolean);
        case FMIStringType:
            return 0;
        case FMIBinaryType:
            return 0;
        case FMIClockType:
            return sizeof(fmi3Clock);
        default:
            return 0;
    }
}

FMISimulationResult* FMICreateSimulationResult(FMIModelDescription* modelDescription) {

    if (!modelDescription) return NULL;

    FMISimulationResult* result = calloc(1, sizeof(FMISimulationResult));

    if (!result) return NULL;

    result->variables = calloc(modelDescription->nModelVariables, sizeof(FMIModelVariable*));

    result->sampleSize = sizeof(double);

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {
        
        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        if (variable->causality != FMIOutput) continue;

        switch (variable->type) {
        case FMIFloat32Type:
        case FMIDiscreteFloat32Type:
        case FMIFloat64Type:
        case FMIDiscreteFloat64Type:
        case FMIInt8Type:
        case FMIUInt8Type:
        case FMIInt16Type:
        case FMIUInt16Type:
        case FMIInt32Type:
        case FMIUInt32Type:
        case FMIInt64Type:
        case FMIUInt64Type:
        case FMIBooleanType:
        case FMIClockType:
            result->variables[result->nVariables++] = variable;
            result->sampleSize += FMISizeForVariableType(variable->type);
            break;
        }
    
    }

    return result;
}

void FMIFreeSimulationResult(FMISimulationResult* result) {
    // TODO
}

FMIStatus FMISample(FMIInstance* instance, double time, FMISimulationResult* result) {

    if (result->dataSize < (result->nSamples + 1) * result->sampleSize) {

        result->dataSize += 64 * result->sampleSize;
        result->data = realloc(result->data, result->dataSize);

        if (!result->data) {
            return FMIFatal;
        }

    }

    FMIStatus status = FMIOK;

    size_t offset = result->nSamples * result->sampleSize;

    *((double*)&result->data[offset]) = time;
    offset += sizeof(double);

    for (size_t i = 0; i < result->nVariables; i++) {

        const FMIModelVariable* variable = result->variables[i];
        const FMIValueReference* vr = &variable->valueReference;

        char* value = &result->data[offset];

        if (instance->fmiVersion == FMIVersion2) {

            switch (variable->type) {
            case FMIRealType:
                CALL(FMI2GetReal(instance, vr, 1, (fmi2Real*)value));
                break;
            case FMIIntegerType:
                CALL(FMI2GetInteger(instance, vr, 1, (fmi2Integer*)value));
                break;
            case FMIBooleanType:
                CALL(FMI2GetBoolean(instance, vr, 1, (fmi2Boolean*)value));
                break;
            default:
                return FMIFatal;
            }

        } else if (instance->fmiVersion == FMIVersion3) {

            switch (variable->type) {
            case FMIFloat32Type:
            case FMIDiscreteFloat32Type:
                CALL(FMI3GetFloat32(instance, vr, 1, (fmi3Float32*)value, 1));
                break;
            case FMIFloat64Type:
            case FMIDiscreteFloat64Type:
                CALL(FMI3GetFloat64(instance, vr, 1, (fmi3Float64*)value, 1));
                break;
            case FMIInt8Type:
                CALL(FMI3GetInt8(instance, vr, 1, (fmi3Int8*)value, 1));
                break;
            case FMIUInt8Type:
                CALL(FMI3GetUInt8(instance, vr, 1, (fmi3UInt8*)value, 1));
                break;
            case FMIInt16Type:
                CALL(FMI3GetInt16(instance, vr, 1, (fmi3Int16*)value, 1));
                break;
            case FMIUInt16Type:
                CALL(FMI3GetUInt16(instance, vr, 1, (fmi3UInt16*)value, 1));
                break;
            case FMIInt32Type:
                CALL(FMI3GetInt32(instance, vr, 1, (fmi3Int32*)value, 1));
                break;
            case FMIUInt32Type:
                CALL(FMI3GetUInt32(instance, vr, 1, (fmi3UInt32*)value, 1));
                break;
            case FMIInt64Type:
                CALL(FMI3GetInt64(instance, vr, 1, (fmi3Int64*)value, 1));
                break;
            case FMIUInt64Type:
                CALL(FMI3GetUInt64(instance, vr, 1, (fmi3UInt64*)value, 1));
                break;
            case FMIBooleanType:
                CALL(FMI3GetBoolean(instance, vr, 1, (fmi3Boolean*)value, 1));
                break;
            case FMIClockType:
                CALL(FMI3GetClock(instance, vr, 1, (fmi3Clock*)value));
                break;
            default:
                return FMIFatal;
            }

        }

        offset += FMISizeForVariableType(variable->type);
    }

    result->nSamples++;

TERMINATE:
    return status;
}

void FMIDumpResult(FMISimulationResult* result, FILE* file) {

    fprintf(file, "\"time\"");

    for (size_t i = 0; i < result->nVariables; i++) {
        fprintf(file, ",\"%s\"", result->variables[i]->name);
    }

    fprintf(file, "\n");

    size_t offset = 0;

    for (size_t i = 0; i < result->nSamples; i++) {

        fprintf(file, "%f", *((double*)&result->data[offset]));
        offset += sizeof(double);

        for (size_t j = 0; j < result->nVariables; j++) {

            FMIModelVariable* variable = result->variables[j];

            const char* value = &result->data[offset];

            switch (variable->type) {
            case FMIFloat32Type:
            case FMIDiscreteFloat32Type:
                fprintf(file, ",%.7g", *((float*)value));
                break;
            case FMIFloat64Type:
            case FMIDiscreteFloat64Type:
                fprintf(file, ",%.16g", *((double*)value));
                break;
            case FMIInt8Type:
                fprintf(file, ",%" PRId8, *((int8_t*)value));
                break;
            case FMIUInt8Type:
                fprintf(file, ",%" PRIu8, *((uint8_t*)value));
                break;
            case FMIInt16Type:
                fprintf(file, ",%" PRId16, *((int16_t*)value));
                break;
            case FMIUInt16Type:
                fprintf(file, ",%" PRIu16, *((uint16_t*)value));
                break;
            case FMIInt32Type:
                fprintf(file, ",%" PRId32, *((int32_t*)value));
                break;
            case FMIUInt32Type:
                fprintf(file, ",%" PRIu32, *((uint32_t*)value));
                break;
            case FMIInt64Type:
                fprintf(file, ",%" PRId64, *((int64_t*)value));
                break;
            case FMIUInt64Type:
                fprintf(file, ",%" PRIu64, *((uint64_t*)value));
                break;
            case FMIBooleanType:
                fprintf(file, ",%d", *((bool*)value));
                break;
            case FMIStringType:
                break;
            case FMIBinaryType:
                break;
            case FMIClockType:
                fprintf(file, ",%d", *((bool*)value));
                break;
            default:
                return FMIFatal;  // unkown variable type
            }

            offset += FMISizeForVariableType(variable->type);

        }
        
        fprintf(file, "\n");

    }

    return FMIOK;
}
