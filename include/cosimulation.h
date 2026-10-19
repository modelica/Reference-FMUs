#pragma once

#include <stddef.h>
#include <stdint.h>

#include "model.h"

Status doFixedStep(ModelInstance *comp, bool* stateEvent, bool* timeEvent);

size_t serializedFMUStateSize(const ModelInstance *comp);

Status serializeFMUState(const ModelInstance *comp, uint8_t *buffer, size_t size);

Status deserializeFMUState(ModelInstance *comp, const uint8_t *buffer, size_t size);
