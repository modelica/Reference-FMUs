#ifndef cosimulation_h
#define cosimulation_h

#include "model.h"

Status doStep(ModelInstance *comp, double t, double tNext, int* earlyReturn, double* lastSuccessfulTime);

#endif /* cosimulation_h */
