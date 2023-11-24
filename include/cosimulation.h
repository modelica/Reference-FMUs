#pragma once

#include "model.h"

#define EPSILON (FIXED_SOLVER_STEP * 1e-6)

Status doFixedStep(ModelInstance *comp, bool* stateEvent, bool* timeEvent);
