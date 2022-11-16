#include <math.h>
#include "csv.h"
#include "FMI2.h"
#include "FMI3.h"
#include "fmusim_input.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMUStaticInput* FMIReadInput(const FMIModelDescription* modelDescription, const char* filename) {

	FMUStaticInput* input = (FMUStaticInput*)calloc(1, sizeof(FMUStaticInput));

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
		input->nVariables++;
	}

	// data
	while (row = CsvReadNextRow(handle)) {

		input->time   = realloc(input->time,   (input->nRows + 1) * sizeof(double));
		input->values = realloc(input->values, (input->nRows + 1) * input->nVariables * sizeof(double));

		char* eptr;

		// time
		col = CsvReadNextCol(row, handle);

		input->time[input->nRows] = strtod(col, &eptr);

		size_t i = 0;

		while (col = CsvReadNextCol(row, handle)) {
			
			if (i >= input->nVariables) {
				printf("The number of columns must be equal to the number of variables.\n");
				return NULL;
			}

			size_t index = (input->nRows * input->nVariables) + i;

			input->values[index] = strtod(col, &eptr);
			
			i++;
		}

		input->nRows++;
	}

	return input;
}

void FMIFreeInput(FMUStaticInput* input) {
	// TODO
}

double FMINextInputEvent(FMUStaticInput* input, double time) {

	if (!input) {
		return INFINITY;
	}

	for (size_t i = 0; i < input->nRows - 1; i++) {
		
		const double t0 = input->time[i];
		const double t1 = input->time[i + 1];

		//if (time < t0) {
		//	continue;
		//}

		if (t0 == t1) {
			return t0;  // discrete change of a continuous variable
		}

		for (size_t j = 0; j < input->nVariables; j++) {
			
			const FMIModelVariable* variable = input->variables[j];
			const FMIVariableType   type     = variable->type;

			if (type == FMIFloat32Type || type == FMIFloat64Type) {
				continue;  // skip continuous variables
			}

			const double v0 = input->values[i * input->nVariables + j];
			const double v1 = input->values[(i + 1) * input->nVariables + j];

			if (v0 != v1) {
				return t1;  // discrete variable change
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

		const FMIModelVariable*  variable = input->variables[i];
		const FMIVariableType    type     = variable->type;
		const fmi2ValueReference vr       = variable->valueReference;
		const double             value    = input->values[(input->nVariables * row) + i];

		double interpolatedValue;

		if (row >= input->nRows - 1) {
			interpolatedValue = input->values[(input->nVariables * row) + i];
		} else {
			const double t0 = input->time[row];
			const double t1 = input->time[row + 1];

			const double x0 = input->values[(input->nVariables * row) + i];
			const double x1 = input->values[(input->nVariables * (row + 1)) + i];

			interpolatedValue = x0 + (time - t0) * (x1 - x0) / (t1 - t0);
		}

		if (instance->fmiVersion == FMIVersion2) {

			if (type == FMIRealType && continuous) {

				CALL(FMI2SetReal(instance, &vr, 1, (fmi2Real*)&interpolatedValue));

			} else if(type == FMIDiscreteRealType && discrete) {

				CALL(FMI2SetReal(instance, &vr, 1, (fmi2Real*)&value));

			} else if (type == FMIIntegerType && discrete) {
			
				const fmi2Integer integerValue = (fmi2Integer)value;
				CALL(FMI2SetInteger(instance, &vr, 1, (fmi2Integer*)&integerValue));
			
			} else if (type == FMIBooleanType && discrete) {
			
				const fmi2Boolean booleanValue = value != fmi2False ? fmi2True : fmi2False;
				CALL(FMI2SetBoolean(instance, &vr, 1, (fmi2Boolean*)&booleanValue));
			
			}

		} else if (instance->fmiVersion == FMIVersion3) {
		
			if (type == FMIFloat64Type && continuous) {

				CALL(FMI3SetFloat64(instance, &vr, 1, (fmi3Float64*)&interpolatedValue, 1));

			} else if (type == FMIDiscreteFloat64Type && discrete) {

				CALL(FMI3SetFloat64(instance, &vr, 1, (fmi3Float64*)&value, 1));

			} else if (type == FMIInt32Type && discrete) {

				const fmi3Int32 int32Value = (fmi2Integer)value;
				CALL(FMI3SetInt32(instance, &vr, 1, (fmi3Int32*)&int32Value, 1));

			} else if (type == FMIBooleanType && discrete) {

				const fmi3Boolean booleanValue = value != fmi3False ? fmi3True : fmi3False;
				CALL(FMI3SetBoolean(instance, &vr, 1, (fmi3Boolean*)&booleanValue, 1));

			}

		}
		
	}

TERMINATE:

	return status;
}
