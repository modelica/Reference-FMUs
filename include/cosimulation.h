#ifndef cosimulation_h
#define cosimulation_h

#include "model.h"

// Internal functions that make up an FMI independent layer.
// These are prefixed to enable static linking.
#define doFixedStep   fmi3FullName(doFixedStep)

void doFixedStep(ModelInstance *comp, bool* stateEvent, bool* timeEvent);

#endif /* cosimulation_h */
