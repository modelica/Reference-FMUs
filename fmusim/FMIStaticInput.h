#pragma once

#include "FMIModelDescription.h"


typedef struct {
	
	FMIMajorVersion fmiMajorVersion;
	size_t nVariables;
	const FMIModelVariable** variables;
	size_t* nValues;
	void** values;
	size_t** sizes;
	size_t nRows;
	double* time;

	void* buffer;
	size_t bufferSize;

} FMIStaticInput;

FMIStaticInput* FMIReadInput(const FMIModelDescription* modelDescription, const char* filename);

void FMIFreeInput(FMIStaticInput* input);

double FMINextInputEvent(const FMIStaticInput* input, double time);

FMIStatus FMIApplyInput(FMIInstance* instance, const FMIStaticInput* input, double time, bool discrete, bool continuous, bool afterEvent);
