#pragma once

#include "model.h"

Status doFixedStep(
    ModelInstance *instance,
    bool *stateEvent,
    bool *timeEvent);

Status doStep(
    ModelInstance *instance,
    double currentCommunicationPoint,
    double communicationStepSize,
    bool noSetFMUStatePriorToCurrentPoint,
    bool *eventHandlingNeeded,
    bool *terminateSimulation,
    bool *earlyReturn,
    double *lastSuccessfulTime);
