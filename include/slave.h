#ifndef slave_h
#define slave_h

#include "model.h"

Status doStep(ModelInstance *comp, double t, double tNext, int* earlyReturn, double* lastSuccessfulTime);

#endif /* slave_h */
