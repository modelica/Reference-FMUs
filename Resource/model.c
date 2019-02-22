#include "config.h"
#include "model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setStartValues(ModelInstance *comp) {
    M(y) = 0;
}

void calculateValues(ModelInstance *comp) {
    // load the file

    FILE *file;
    char *path;
    char c;
    const char *scheme1 = "file:///";
    const char *scheme2 = "file:/";
#if FMI_VERSION < 2
    char *resourcePath = "/resources/y.txt";
#else
    char *resourcePath = "/y.txt";
#endif

    if (!comp->resourceLocation) {
        // FMI 1.0 for Model Exchange doesn't have a resource location
        return;
    }

    if (strncmp(comp->resourceLocation, scheme1, strlen(scheme1)) == 0) {
        path = malloc(strlen(comp->resourceLocation) + strlen(resourcePath) + 1);
        strcpy(path, &comp->resourceLocation[strlen(scheme1)] - 1);
        strcat(path, resourcePath);
    } else if (strncmp(comp->resourceLocation, scheme2, strlen(scheme2)) == 0) {
        path = malloc(strlen(comp->resourceLocation) + strlen(resourcePath) + 1);
        strcpy(path, &comp->resourceLocation[strlen(scheme2) - 1]);
    } else {
        logError(comp, "The resourceLocation must start with \"file:/\" or \"file:///\"");
        return;
    }

#ifdef _WIN32
	// strip any leading slashes
	while (path[0] == '/') {
		strcpy(path, &path[1]);
	}
#endif

    // open the resource file
    file = fopen (path, "r");

    if (!file) {
        logError(comp, "Failed to open resource file %s.", path);
        return;
    }

    // read the first character
    c = fgetc(file);

    // assign it to y
    comp->modelData->y = c;

    // clost the file
    fclose(file);
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    switch (vr) {
        case vr_y:
            *value = M(y);
            return OK;
        default:
            return Error;
    }
}

void eventUpdate(ModelInstance *comp) {
    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;
}
