#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "csv.h"
#include "FMIUtil.h"
#include "FMIStaticInput.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStaticInput* FMIReadInput(const FMIModelDescription* modelDescription, const char* filename) {

	FMIStatus status = FMIOK;

	FMIStaticInput* input = NULL;

    CsvHandle handle = NULL;

	CALL(FMICalloc((void**)&input, 1, sizeof(FMIStaticInput)));
	
	input->fmiMajorVersion = modelDescription->fmiMajorVersion;

	char* row = NULL;

    handle = CsvOpen(filename);

	if (!handle) {
		FMILogError("Failed to read input file %s.\n", filename);
        status = FMIError;
        goto TERMINATE;
	}

	// variable names
	row = CsvReadNextRow(handle);

    const char* col = CsvReadNextCol(row, handle);

    while ((col = CsvReadNextCol(row, handle))) {

		const FMIModelVariable* variable = FMIModelVariableForName(modelDescription, col);

		if (!variable) {
            FMILogError("Variable %s not found.\n", col);
            status = FMIError;
            goto TERMINATE;
        }

		CALL(FMIRealloc((void**)&input->variables, (input->nVariables + 1) * sizeof(FMIModelVariable*)));
		input->variables[input->nVariables] = variable;
		input->nVariables++;
	}

	// data
    while ((row = CsvReadNextRow(handle))) {

        CALL(FMIRealloc((void**)&input->time, (input->nRows + 1) * sizeof(double)));
		CALL(FMIRealloc((void**)&input->nValues, (input->nRows + 1) * input->nVariables * sizeof(size_t)));
		CALL(FMIRealloc((void**)&input->values, (input->nRows + 1) * input->nVariables * sizeof(void*)));
		CALL(FMIRealloc((size_t**)&input->sizes, (input->nRows + 1) * input->nVariables * sizeof(size_t*)));

		const size_t index = input->nRows * input->nVariables;
		memset(&input->nValues[index], 0x0, input->nVariables * sizeof(size_t));
		memset(&input->values [index], 0x0, input->nVariables * sizeof(void*));

		char* eptr;

		// time
		col = CsvReadNextCol(row, handle);

		input->time[input->nRows] = strtod(col, &eptr);

        if (input->nRows > 0 && input->time[input->nRows - 1] > input->time[input->nRows]) {
            FMILogError("Values in first column (time) must be monotonically increasing.\n");
            status = FMIError;
            goto TERMINATE;
        }

		size_t i = 0; // variable index

        while ((col = CsvReadNextCol(row, handle))) {
			
			if (i >= input->nVariables) {
                FMILogError("The number of columns must be equal to the number of variables.\n");
                status = FMIError;
                goto TERMINATE;
            }

			const FMIModelVariable* variable = input->variables[i];

			const size_t index = (input->nRows * input->nVariables) + i;

			CALL(FMIParseValues(modelDescription->fmiMajorVersion, variable->type, col, &input->nValues[index], &input->values[index], &input->sizes[index]));
			
			i++;
		}

		input->nRows++;
	}

TERMINATE:

    if (handle) {
        CsvClose(handle);
    }

	if (status == FMIOK) {
		return input;
	}

	FMIFreeInput(input);

	return NULL;
}

void FMIFreeInput(FMIStaticInput* input) {

	if (!input) {
		return;
	}

	for (size_t i = 0; i < input->nRows; i++) {

		for (size_t j = 0; j < input->nVariables; j++) {

			const FMIModelVariable* variable = input->variables[j];
			const FMIVariableType   type = variable->type;
			const size_t            nValues = input->nValues[j];
		
			const size_t k = i * input->nVariables + j;

			if (type == FMIStringType) {
				const char** values = input->values[k];
				FMIFree((void**)&values);
			} else if (type == FMIBinaryType) {
				size_t* sizes = input->sizes[k];
				FMIFree((void**)&sizes);
				void** values = input->values[k];
				FMIFree((void**)&values);
			}
		
		}

	}

	FMIFree((void**)&input->variables);
	FMIFree((void**)&input->nValues);
	FMIFree((void**)&input->values);
	FMIFree((void**)&input->sizes);
	FMIFree((void**)&input->buffer);

	FMIFree((void**)&input);
}

double FMINextInputEvent(const FMIStaticInput* input, double time) {

	if (!input) {
		return INFINITY;
	}

	for (size_t i = 0; i < input->nRows - 1; i++) {
		
		const double t0 = input->time[i];
		const double t1 = input->time[i + 1];

		if (time > t1 || FMIIsClose(time, t1)) {
			continue;
		}

		if (t0 == t1) {
			return t0;  // discrete change of a continuous variable
		}

		for (size_t j = 0; j < input->nVariables; j++) {
			
			const FMIModelVariable* variable = input->variables[j];
			const FMIVariableType   type     = variable->type;
			const size_t            nValues = input->nValues[j];

			if (variable->variability == FMIContinuous) {
				continue;  // skip continuous variables
			}

			const size_t k0 = i * input->nVariables + j;
			const size_t k1 = (i + 1) * input->nVariables + j;

			const size_t nValues0 = input->nValues[k0];
			const size_t nValues1 = input->nValues[k1];

			if (nValues0 != nValues1) {
				continue;  // array sizes have changed
			}

			const void* values0 = input->values[k0];
			const void* values1 = input->values[k1];
						
			if (type == FMIBinaryType) {

				const size_t* sizes0 = input->sizes[k0];
				const size_t* sizes1 = input->sizes[k1];

				for (size_t l = 0; l < nValues; l++) {

					const void* c0 = ((void**)values0)[l];
					const void* c1 = ((void**)values1)[l];

					const size_t size0 = sizes0[l];
					const size_t size1 = sizes1[l];

					if (size0 != size1 || memcmp(c0, c1, size0)) {
						return t1;
					}
				}

			} else if (type == FMIStringType) {

				for (size_t l = 0; l < nValues; l++) {

					const char* c0 = ((char**)values0)[l];
					const char* c1 = ((char**)values1)[l];

					if (strcmp(c0, c1)) {
						return t1;
					}
				}
					
			} else {

				size_t size = FMISizeOfVariableType(type, input->fmiMajorVersion) * nValues;

				if (memcmp(values0, values1, size)) {
					return t1;
				}
			}


		}

	}

	return INFINITY;
}

FMIStatus FMIApplyInput(FMIInstance* instance, const FMIStaticInput* input, double time, bool discrete, bool continuous, bool afterEvent) {

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

			while (row < input->nRows - 1) {

				const double nextTime = input->time[row + 1];

				if (afterEvent ? nextTime > time : nextTime >= time) {
					break;
				}

				row++;
			}

			const size_t j = row * input->nVariables + i;

			const size_t nValues = input->nValues[j];

			if (nValues == 0) continue;

            const size_t requiredBufferSize = nValues * sizeof(double);

			if (input->bufferSize < requiredBufferSize) {
				// TODO: allocate in FMIReadInput()
				CALL(FMIRealloc((void**)&input->buffer, requiredBufferSize));
				((FMIStaticInput*)input)->bufferSize = requiredBufferSize;
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

                        buffer[k] = (float)(x0 + (time - t0) * (x1 - x0) / (t1 - t0));
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

			CALL(FMISetValues(instance, type, &vr, 1, NULL, input->buffer, nValues));

		} else if (discrete && (variable->variability == FMIDiscrete || variable->variability == FMITunable)) {

			size_t row = 0;

			for (size_t j = 1; j < input->nRows; j++) {

				const double t = input->time[j];

				if (t > time || FMIIsClose(t, time)) {
					break;
				}

				row = j;
			}

			if (afterEvent) {

				while (row < input->nRows - 1) {

					const double t = input->time[row + 1];

					if (t > time && !FMIIsClose(t, time)) {
						break;
					}

					row++;
				}
			}

			const size_t j = row * input->nVariables + i;

			const size_t nValues = input->nValues[j];
			const void* values = input->values[j];
			const size_t* sizes = input->sizes[j];

			if (nValues == 0) {
				continue;
			}

			if (variable->type == FMIClockType) {

				if (instance->state == FMIInitializationModeState) {
					continue; // don't set clocks in Initialization Mode
				}

				if (!*((bool*)values)) {
					continue;  // don't set inactive clocks
				}

			}
	
			CALL(FMISetValues(instance, type, &vr, 1, sizes, values, nValues));
		}
	}

TERMINATE:

	return status;
}
