#pragma once

#include "model.h"

Status doFixedStep(
    ModelInstance *comp,
    bool* stateEvent,
    bool* timeEvent
);

Status doStep(
    ModelInstance* comp,
    double currentCommunicationPoint,
    double communicationStepSize,
    bool noSetFMUStatePriorToCurrentPoint,
    bool* eventHandlingNeeded,
    bool* terminateSimulation,
    bool* earlyReturn,
    double* lastSuccessfulTime
);
