#pragma once

#include "FMI.h"

typedef struct FMIRecorderImpl FMIRecorder;

typedef FMIStatus (*FMIRecorderSample)(FMIRecorder* recorder, double time);
