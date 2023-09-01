#include <math.h>
#include "csv.h"
#include "FMI1.h"
#include "FMI2.h"
#include "FMI3.h"
#include "FMIUtil.h"
#include "fmusim_input.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


//static size_t FMIGetNumberOfValues(const FMIModelDescription* modelDescription, const FMIModelVariable* variable) {
//
//	size_t nValues = 1;
//
//	for (size_t i = 0; i < variable->nDimensions; i++) {
//		
//		const FMIDimension* dimension = &variable->dimensions[i];
//
//		size_t n;
//
//		if (dimension->variable) {
//			n = atoi(dimension->variable->start);
//		} else {
//			n = dimension->start;
//		}
//
//		nValues *= n;
//
//	}
//
//	return nValues;
//}

FMUStaticInput* FMIReadInput(const FMIModelDescription* modelDescription, const char* filename) {

	FMUStaticInput* input = (FMUStaticInput*)calloc(1, sizeof(FMUStaticInput));

	input->fmiVersion = modelDescription->fmiVersion;

	char* row = NULL;
	int cols = 0;

	CsvHandle handle = CsvOpen(filename);

	// variable names
	row = CsvReadNextRow(handle);

	const char* col = CsvReadNextCol(row, handle);

	while (col = CsvReadNextCol(row, handle)) {

		const FMIModelVariable* variable = FMIModelVariableForName(modelDescription, col);

		if (!variable) {
			printf("Variable %s not found.\n", col);
			return NULL;
		}

		input->variables = realloc(input->variables, (input->nVariables + 1) * sizeof(FMIModelVariable*));
		input->variables[input->nVariables] = variable;
		//const size_t nValues = FMIGetNumberOfValues(modelDescription, variable);
		//input->nValues[input->nVariables] = nValues;
		input->nVariables++;
	}

	// data
	while (row = CsvReadNextRow(handle)) {

		input->time    = realloc(input->time,    (input->nRows + 1) * sizeof(double));
		input->nValues = realloc(input->nValues, (input->nRows + 1) * input->nVariables * sizeof(size_t));
		input->values  = realloc(input->values,  (input->nRows + 1) * input->nVariables * sizeof(void*));
		
		memset(&input->values[input->nRows * input->nVariables], 0x0, input->nVariables * sizeof(void*));

		char* eptr;

		// time
		col = CsvReadNextCol(row, handle);

		input->time[input->nRows] = strtod(col, &eptr);

		size_t i = 0; // variable index

		while (col = CsvReadNextCol(row, handle)) {
			
			if (i >= input->nVariables) {
				printf("The number of columns must be equal to the number of variables.\n");
				return NULL;
			}

			const FMIModelVariable* variable = input->variables[i];

			const size_t index = (input->nRows * input->nVariables) + i;

			eptr = col;
			size_t j = 0; // value index

			while (strlen(eptr) > 0) {
				
				switch (variable->type) {
				case FMIFloat32Type:
				case FMIDiscreteFloat32Type:
					input->values[index] = realloc(input->values[index], sizeof(fmi3Float32) * (j + 1));
					((fmi3Float32*)input->values[index])[j] = strtof(eptr, &eptr);
					break;
				case FMIFloat64Type:
				case FMIDiscreteFloat64Type:
					input->values[index] = realloc(input->values[index], sizeof(fmi3Float64) * (j + 1));
					((fmi3Float64*)input->values[index])[j] = strtod(eptr, &eptr);
					break;
				case FMIInt8Type:
					input->values[index] = realloc(input->values[index], sizeof(fmi3Int8) * (j + 1));
					((fmi3Int8*)input->values[index])[j] = strtol(eptr, &eptr, 10);
					break;
				case FMIUInt8Type:
					input->values[index] = realloc(input->values[index], sizeof(fmi3UInt8) * (j + 1));
					((fmi3UInt8*)input->values[index])[j] = strtoul(eptr, &eptr, 10);
					break;
				case FMIInt16Type:
					input->values[index] = realloc(input->values[index], sizeof(fmi3Int16) * (j + 1));
					((fmi3Int16*)input->values[index])[j] = strtol(eptr, &eptr, 10);
					break;
				case FMIUInt16Type:
					input->values[index] = realloc(input->values[index], sizeof(fmi3UInt16) * (j + 1));
					((fmi3UInt16*)input->values[index])[j] = strtoul(eptr, &eptr, 10);
					break;
				case FMIInt32Type:
					input->values[index] = realloc(input->values[index], sizeof(fmi3Int32) * (j + 1));
					((fmi3Int32*)input->values[index])[j] = strtol(eptr, &eptr, 10);
					break;
				case FMIUInt32Type:
					input->values[index] = realloc(input->values[index], sizeof(fmi3UInt32) * (j + 1));
					((fmi3UInt32*)input->values[index])[j] = strtoul(eptr, &eptr, 10);
					break;
				case FMIInt64Type:
					input->values[index] = realloc(input->values[index], sizeof(fmi3Int64) * (j + 1));
					((fmi3Int64*)input->values[index])[j] = strtol(eptr, &eptr, 10);
					break;
				case FMIUInt64Type:
					input->values[index] = realloc(input->values[index], sizeof(fmi3UInt64) * (j + 1));
					((fmi3UInt64*)input->values[index])[j] = strtoul(eptr, &eptr, 10);
					break;
				case FMIBooleanType:
					if (modelDescription->fmiVersion == FMIVersion1) {
						input->values[index] = realloc(input->values[index], sizeof(fmi1Boolean) * (j + 1));
						((fmi1Boolean*)input->values[index])[j] = strtol(eptr, &eptr, 10);
					} else if (modelDescription->fmiVersion == FMIVersion2) {
						input->values[index] = realloc(input->values[index], sizeof(fmi2Boolean) * (j + 1));
						((fmi2Boolean*)input->values[index])[j] = strtol(eptr, &eptr, 10);
					} else if (modelDescription->fmiVersion == FMIVersion3) {
						input->values[index] = realloc(input->values[index], sizeof(fmi3Boolean) * (j + 1));
						((fmi3Boolean*)input->values[index])[j] = strtol(eptr, &eptr, 10);
					}
					break;
				case FMIClockType:
					input->values[index] = realloc(input->values[index], sizeof(fmi3Clock) * (j + 1));
					((fmi3Clock*)input->values[index])[j] = strtol(eptr, &eptr, 10);
					break;
				default:
					printf("Unspported input variable type.\n");
					return NULL;
				}
				
				j++;
			}

			input->nValues[index] = j;
			
			i++;
		}

		input->nRows++;
	}

	return input;
}

void FMIFreeInput(FMUStaticInput* input) {
	// TODO
}

static size_t FMISizeOf(FMIVariableType type, FMIVersion fmiVersion) {

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
		switch (fmiVersion) {
		case FMIVersion1:
			return sizeof(fmi1Boolean);
		case FMIVersion2:
			return sizeof(fmi2Boolean);
		case FMIVersion3:
			return sizeof(fmi3Boolean);
		default: 
			return 0;
		}
	
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

double FMINextInputEvent(const FMUStaticInput* input, double time) {

	if (!input) {
		return INFINITY;
	}

	for (size_t i = 0; i < input->nRows - 1; i++) {
		
		const double t0 = input->time[i];
		const double t1 = input->time[i + 1];

		if (time >= t1) {
			continue;
		}

		if (t0 == t1) {
			return t0;  // discrete change of a continuous variable
		}

		for (size_t j = 0; j < input->nVariables; j++) {
			
			const FMIModelVariable* variable = input->variables[j];
			const FMIVariableType   type     = variable->type;
			const size_t            nValues = input->nValues[j];

		    if (type == FMIFloat32Type || type == FMIFloat64Type) {
				continue;  // skip continuous variables
			}

			const void* values0 = input->values[i * input->nVariables + j];
			const void* values1 = input->values[(i + 1) * input->nVariables + j];
			const size_t size = FMISizeOf(type, input->fmiVersion) * nValues;

			if (memcmp(values0, values1, size)) {
				return t1;
			}

			//if (type == FMIFloat32Type || type == FMIFloat64Type) {
			//	continue;  // skip continuous variables
			//}

			//const double v0 = 0; // input->values[i * input->nVariables + j];
			//const double v1 = 0; // input->values[(i + 1) * input->nVariables + j];

			//if (v0 != v1) {
			//	return t1;  // discrete variable change
			//}
		}

	}

	return INFINITY;
}

FMIStatus FMIApplyInput(FMIInstance* instance, FMUStaticInput* input, double time, bool discrete, bool continuous, bool afterEvent) {

	FMIStatus status = FMIOK;

	if (!input) {
		goto TERMINATE;
	}

	size_t row = 0;

	for (size_t i = 1; i < input->nRows; i++) {

		const double t = input->time[i];

		if (t >= time) {
			break;
		}

		row = i;
	}

	if (afterEvent) {

		while (row < input->nRows - 2) {

			if (input->time[row + 1] > time) {
				break;
			}

			row++;
		}
	}

	for (size_t i = 0; i < input->nVariables; i++) {

		const FMIModelVariable* variable = input->variables[i];
		const FMIVariableType   type     = variable->type;
		const FMIValueReference vr       = variable->valueReference;

		const size_t j = row * input->nVariables + i;

		const size_t nValues = input->nValues[j];
		const void* values   = input->values[j];

		if (continuous && variable->variability == FMIContinuous) {
			
			const size_t requiredBufferSize = nValues * sizeof(fmi3Float64);

			if (input->bufferSize < requiredBufferSize) {
				input->buffer = realloc(input->buffer, requiredBufferSize);
				input->bufferSize = requiredBufferSize;
			}

			if (variable->type == FMIFloat32Type) {

				float* buffer = (double*)input->buffer;

				const float* values0 = (float*)input->values[row * input->nVariables + i];
				const float* values1 = (float*)input->values[(row + 1) * input->nVariables + i];

				for (size_t k = 0; k < nValues; k++) {

					float interpolatedValue;

					if (row >= input->nRows - 1) {
						interpolatedValue = values0[k];
					} else {
						const double t0 = input->time[row];
						const double t1 = input->time[row + 1];

						const double x0 = values0[k];
						const double x1 = values1[k];

						interpolatedValue = x0 + (time - t0) * (x1 - x0) / (t1 - t0);
					}

					buffer[k] = interpolatedValue;

				}

			} else if (variable->type == FMIFloat64Type) {

				double* buffer = (double*)input->buffer;

				const double* values0 = (double*)input->values[row * input->nVariables + i];
				const double* values1 = (double*)input->values[(row + 1) * input->nVariables + i];

				for (size_t k = 0; k < nValues; k++) {

					if (row >= input->nRows - 1) {
						buffer[k] = values0[k];
					} else {
						const double t0 = input->time[row];
						const double t1 = input->time[row + 1];

						const double x0 = values0[k];
						const double x1 = values1[k];

						buffer[k] = x0 + (time - t0) * (x1 - x0) / (t1 - t0);
					}

				}

			}

			if (instance->fmiVersion == FMIVersion1) {
				CALL(FMI1SetValues(instance, type, &vr, 1, input->buffer));
			} else if (instance->fmiVersion == FMIVersion2) {
				CALL(FMI2SetValues(instance, type, &vr, 1, input->buffer));
			} else if (instance->fmiVersion == FMIVersion3) {
				CALL(FMI3SetValues(instance, type, &vr, 1, input->buffer, nValues));
			}

		} else if (discrete && (variable->variability == FMIDiscrete || variable->variability == FMITunable)) {

			if (instance->fmiVersion == FMIVersion1) {
				CALL(FMI1SetValues(instance, type, &vr, 1, values));
			} else if (instance->fmiVersion == FMIVersion2) {
				CALL(FMI2SetValues(instance, type, &vr, 1, values));
			} else if (instance->fmiVersion == FMIVersion3) {
				CALL(FMI3SetValues(instance, type, &vr, 1, values, nValues));
			}

		}

	}

TERMINATE:

	return status;
}
