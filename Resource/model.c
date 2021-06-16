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

void setStartValues(ModelInstance *comp) {
    M(y) = 0;
}

void calculateValues(ModelInstance *comp) {
    // load the file

    FILE *file = NULL;
    char path[MAX_PATH_LENGTH] = "";
    char c = '\0';
    const char *scheme1 = "file:///";
    const char *scheme2 = "file:/";

    if (!comp->resourceLocation) {
        // FMI 1.0 for Model Exchange doesn't have a resource location
        return;
    }

#ifdef _WIN32
    DWORD pathLen = MAX_PATH_LENGTH;

    if (PathCreateFromUrl(comp->resourceLocation, path, &pathLen, 0) != S_OK) {
        return;
    }

#if FMI_VERSION < 2
    if (!PathAppend(path, "resources")) return;
#endif

    if (!PathAppend(path, "y.txt")) return;

#else

    if (strncmp(comp->resourceLocation, scheme1, strlen(scheme1)) == 0) {
        strcpy(path, &comp->resourceLocation[strlen(scheme1)] - 1);
    } else if (strncmp(comp->resourceLocation, scheme2, strlen(scheme2)) == 0) {
        strcpy(path, &comp->resourceLocation[strlen(scheme2) - 1]);
    } else {
        logError(comp, "The resourceLocation must start with \"file:/\" or \"file:///\"");
        return;
    }

#if FMI_VERSION < 2
    strcat(path, "/resources/y.txt");
#else
    strcat(path, "/y.txt");
#endif

#endif

    // open the resource file
    file = fopen (path, "r");

    if (!file) {
        logError(comp, "Failed to open resource file %s.", path);
        return;
    }

    // read the first character
    c = (char)fgetc(file);

    // assign it to y
    comp->modelData->y = c;

    // close the file
    fclose(file);
}


Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    switch (vr) {
    case vr_time:
        value[(*index)++] = comp->time;
        return OK;
    default:
        logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
        return Error;
    }
}


Status getInt32(ModelInstance* comp, ValueReference vr, int *value, size_t *index) {
    switch (vr) {
        case vr_y:
            value[(*index)++] = M(y);
            return OK;
        default:
            logError(comp, "Get Int32 is not allowed for value reference %u.", vr);
            return Error;
    }
}

void eventUpdate(ModelInstance *comp) {
    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;
}
