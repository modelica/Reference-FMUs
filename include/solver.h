#ifndef solver_h
#define solver_h

#include "config.h"
#include "model.h"


void *solver_create(ModelInstance *comp);
void solver_step(ModelInstance *comp, double t, double tNext, double *tRet, int *stateEvent);
void solver_reset(ModelInstance *comp);


#endif /* solver_h */
