#pragma once

#include "FMIModelDescription.h"


typedef struct {
	
	size_t nVariables;
	const FMIModelVariable** variables;
	size_t nRows;
	double* time;
	double* values;

} FMUStaticInput;

FMUStaticInput* FMIReadInput(const FMIModelDescription* modelDescription, const char* filename);

void FMIFreeInput(FMUStaticInput* input);

double FMINextInputEvent(const FMUStaticInput* input, double time);

FMIStatus FMIApplyInput(FMIInstance* instance, const FMUStaticInput* input, double time, bool discrete, bool continuous, bool afterEvent);
