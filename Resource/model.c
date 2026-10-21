#include "config.h"
#include "model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")
#endif

#define MAX_PATH_LENGTH 4096

Status setStartValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    M(y) = 0;

    comp->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    FILE *file = NULL;
    char path[MAX_PATH_LENGTH] = "";
    char c = '\0';

    if (!comp->resourceLocation) {
        logError(comp, "Resource location must not be NULL.");
        return Error;
    }

#ifdef _WIN32

    strncpy(path, comp->resourceLocation, MAX_PATH_LENGTH);
    path[MAX_PATH_LENGTH - 1] = '\0';

#if FMI_VERSION == 2
    if (!PathAppendA(path, "y.txt")) return Error;
#else
    if (!strncat(path, "y.txt", MAX_PATH_LENGTH)) return Error;
#endif

#else

    strncpy(path, comp->resourceLocation, MAX_PATH_LENGTH);
    path[MAX_PATH_LENGTH - 1] = '\0';

#if FMI_VERSION == 2
    strncat(path, "/y.txt", MAX_PATH_LENGTH-strlen(path)-1);
#else
    strncat(path, "y.txt", MAX_PATH_LENGTH-strlen(path)-1);
#endif
    path[MAX_PATH_LENGTH-1] = 0;
#endif

    // open the resource file
    file = fopen (path, "r");

    if (!file) {
        logError(comp, "Failed to open resource file %s.", path);
        return Error;
    }

    // read the first character
    c = (char)fgetc(file);

    // assign it to y
    M(y) = c;

    // close the file
    fclose(file);

    comp->isDirtyValues = false;

    return OK;
}


Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(comp);

    switch (vr) {
    case vr_time:
        ASSERT_NVALUES(1);
        values[(*index)++] = comp->time;
        return OK;
    default:
        logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
        return Error;
    }
}


Status getInt32(ModelInstance* comp, ValueReference vr, int32_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(comp);

    switch (vr) {
        case vr_y:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(y);
            return OK;
        default:
            logError(comp, "Get Int32 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status eventUpdate(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;

    comp->isDirtyValues = true;

    return OK;
}
