#include "csv.h"
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

		FMIModelVariable* variable = FMIModelVariableForName(modelDescription, col);

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

FMIStatus FMIApplyInput(FMIInstance* instance, FMUStaticInput* input, double time, bool discrete, bool continuous, bool afterEvent) {

	FMIStatus status = FMIOK;

	size_t row = 0;

	for (size_t i = 1; i < input->nRows; i++) {

		const double t = input->time[i];

		if (t > time) {
			break;
		}

		row = i;
	}

	for (size_t i = 0; i < input->nVariables; i++) {

		const FMIModelVariable*  variable    = input->variables[i];
		const FMIVariableType    type        = variable->type;
		const fmi2ValueReference vr          = variable->valueReference;
		const double             doubleValue = input->values[(input->nVariables * row) + i];

		if (instance->fmiVersion == FMIVersion2) {

			if ((type == FMIRealType && continuous) || (type == FMIDiscreteRealType && discrete)) {

				CALL(FMI2SetReal(instance, &vr, 1, (fmi2Real*)&doubleValue));
			
			} else if (type == FMIIntegerType && discrete) {
			
				const fmi2Integer integerValue = (fmi2Integer)doubleValue;
				CALL(FMI2SetInteger(instance, &vr, 1, (fmi2Integer*)&integerValue));
			
			} else if (type == FMIBooleanType && discrete) {
			
				const fmi2Boolean booleanValue = doubleValue != fmi2False ? fmi2True : fmi2False;
				CALL(FMI2SetBoolean(instance, &vr, 1, (fmi2Boolean*)&booleanValue));
			
			}

		}
		
	}

TERMINATE:

	return status;
}
