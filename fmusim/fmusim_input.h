#pragma once

#include "FMIModelDescription.h"

//typedef struct {
//
//	FMIModelVariable* variable;
//	size_t* nValues;
//	void** values;
//
//} FMIStaticInputColumn;

typedef struct {
	
	FMIVersion fmiVersion;
	size_t nVariables;
	const FMIModelVariable** variables;
	size_t* nValues;
	void** values;
	size_t nRows;
	double* time;

	void* buffer;
	size_t bufferSize;

} FMUStaticInput;

FMUStaticInput* FMIReadInput(const FMIModelDescription* modelDescription, const char* filename);

void FMIFreeInput(FMUStaticInput* input);

double FMINextInputEvent(const FMUStaticInput* input, double time);

FMIStatus FMIApplyInput(FMIInstance* instance, FMUStaticInput* input, double time, bool discrete, bool continuous, bool afterEvent);
