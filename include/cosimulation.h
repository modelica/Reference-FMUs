#ifndef cosimulation_h
#define cosimulation_h

#include "model.h"

Status doStep(ModelInstance *comp,
              double t,
              double tNext,
              bool* eventEncountered,
              bool* terminateSimulation,
              bool* earlyReturn,
              double* lastSuccessfulTime);

#endif /* cosimulation_h */
