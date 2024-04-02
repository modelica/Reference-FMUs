#include <string.h>
#include <math.h>
#include "csv.h"
#include "FMI1.h"
#include "FMI2.h"
#include "FMI3.h"
#include "FMIUtil.h"
#include "fmusim_input.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMUStaticInput* FMIReadInput(const FMIModelDescription* modelDescription, const char* filename) {

	FMIStatus status = FMIOK;

	FMUStaticInput* input = NULL;

	CALL(FMICalloc((void**)&input, 1, sizeof(FMUStaticInput)));
	
	input->fmiVersion = modelDescription->fmiVersion;

	char* row = NULL;
	int cols = 0;

	CsvHandle handle = CsvOpen(filename);

	if (!handle) {
		status = FMIError;
		FMILogError("Failed to read input file %s.\n", filename);
		goto TERMINATE;
	}

	// variable names
	row = CsvReadNextRow(handle);

	const char* col = CsvReadNextCol(row, handle);

	while (col = CsvReadNextCol(row, handle)) {

		const FMIModelVariable* variable = FMIModelVariableForName(modelDescription, col);

		if (!variable) {
			FMILogError("Variable %s not found.\n", col);
			return NULL;
		}

		CALL(FMIRealloc((void**)&input->variables, (input->nVariables + 1) * sizeof(FMIModelVariable*)));
		input->variables[input->nVariables] = variable;
		input->nVariables++;
	}

	// data
	while (row = CsvReadNextRow(handle)) {

		CALL(FMIRealloc((void**)&input->time,    (input->nRows + 1) * sizeof(double)));
		CALL(FMIRealloc((void**)&input->nValues, (input->nRows + 1) * input->nVariables * sizeof(size_t)));
		CALL(FMIRealloc((void**)&input->values, (input->nRows + 1) * input->nVariables * sizeof(void*)));
		
		const size_t index = input->nRows * input->nVariables;
		memset(&input->nValues[index], 0x0, input->nVariables * sizeof(size_t));
		memset(&input->values [index], 0x0, input->nVariables * sizeof(void*));

		char* eptr;

		// time
		col = CsvReadNextCol(row, handle);

		input->time[input->nRows] = strtod(col, &eptr);

		size_t i = 0; // variable index

		while (col = CsvReadNextCol(row, handle)) {
			
			if (i >= input->nVariables) {
				FMILogError("The number of columns must be equal to the number of variables.\n");
				return NULL;
			}

			const FMIModelVariable* variable = input->variables[i];

			const size_t index = (input->nRows * input->nVariables) + i;

			CALL(FMIParseValues(modelDescription->fmiVersion, variable->type, col, &input->nValues[index], &input->values[index]));
			
			i++;
		}

		input->nRows++;
	}

TERMINATE:

	if (status == FMIOK) {
		return input;
	}

	FMIFreeInput(input);

	return NULL;
}

void FMIFreeInput(FMUStaticInput* input) {

	if (!input) {
		return;
	}

	FMIFree((void**)&input->variables);
	FMIFree((void**)&input->nValues);
	FMIFree((void**)&input->buffer);

	FMIFree((void**)&input);
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
		}

	}

	return INFINITY;
}

FMIStatus FMIApplyInput(FMIInstance* instance, const FMUStaticInput* input, double time, bool discrete, bool continuous, bool afterEvent) {

	FMIStatus status = FMIOK;

	if (!input) {
		goto TERMINATE;
	}

	for (size_t i = 0; i < input->nVariables; i++) {

		const FMIModelVariable* variable = input->variables[i];
		const FMIVariableType   type     = variable->type;
		const FMIValueReference vr       = variable->valueReference;

		if (continuous && variable->variability == FMIContinuous) {

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

			const size_t j = row * input->nVariables + i;

			const size_t nValues = input->nValues[j];
			const void* values = input->values[j];

			if (nValues == 0) continue;

			const size_t requiredBufferSize = nValues * sizeof(fmi3Float64);

			if (input->bufferSize < requiredBufferSize) {
				// TODO: allocate in FMIReadInput()
				CALL(FMIRealloc((void**)&input->buffer, requiredBufferSize));
				((FMUStaticInput*)input)->bufferSize = requiredBufferSize;
			}

			if (variable->type == FMIFloat32Type) {

				float* buffer = (float*)input->buffer;

				const float* values0 = (float*)input->values[row * input->nVariables + i];
				const float* values1 = (float*)input->values[(row + 1) * input->nVariables + i];

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

			size_t row = 0;

			for (size_t i = 1; i < input->nRows; i++) {

				const double t = input->time[i];

				if (t >= time) {
					break;
				}

				row = i;
			}

			if (afterEvent) {

				while (row < input->nRows - 1) {

					if (input->time[row + 1] > time) {
						break;
					}

					row++;
				}
			}

			const size_t j = row * input->nVariables + i;

			const size_t nValues = input->nValues[j];
			const void* values = input->values[j];

			if (nValues == 0) continue;

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
