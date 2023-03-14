#include "config.h"
#include "model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")
#else
#include <ctype.h>
#endif

#define MAX_PATH_LENGTH 4096

void setStartValues(ModelInstance *comp) {
    M(y) = 0;
}

#ifndef _WIN32
// taken from https://stackoverflow.com/questions/2673207/c-c-url-decode-library
void urldecode2(char *dst, const char *src) {
    char a, b;

    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';

            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';

            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }

    *dst++ = '\0';
}
#endif

Status calculateValues(ModelInstance *comp) {

    // load the file
    FILE *file = NULL;
    char path[MAX_PATH_LENGTH] = "";
    char c = '\0';

    if (!comp->resourceLocation) {
        logError(comp, "Resource location must not be NULL.");
        return Error;
    }

#ifdef _WIN32

#if FMI_VERSION < 3
    DWORD pathLen = MAX_PATH_LENGTH;

    if (PathCreateFromUrlA(comp->resourceLocation, path, &pathLen, 0) != S_OK) {
        logError(comp, "Failed to convert resource location to file system path.");
        return Error;
    }
#else
    strncpy(path, comp->resourceLocation, MAX_PATH_LENGTH);
#endif

#if FMI_VERSION == 1
    if (!PathAppendA(path, "resources") || !PathAppendA(path, "y.txt")) return Error;
#elif FMI_VERSION == 2
    if (!PathAppendA(path, "y.txt")) return Error;
#else
    if (!strncat(path, "y.txt", MAX_PATH_LENGTH)) return Error;
#endif

#else

#if FMI_VERSION < 3
    const char *scheme1 = "file:///";
    const char *scheme2 = "file:/";

    if (strncmp(comp->resourceLocation, scheme1, strlen(scheme1)) == 0) {
        strncpy(path, &comp->resourceLocation[strlen(scheme1)] - 1, MAX_PATH_LENGTH-1);
    } else if (strncmp(comp->resourceLocation, scheme2, strlen(scheme2)) == 0) {
        strncpy(path, &comp->resourceLocation[strlen(scheme2) - 1], MAX_PATH_LENGTH-1);
    } else {
        logError(comp, "The resourceLocation must start with \"file:/\" or \"file:///\"");
        return Error;
    }

    urldecode2(path, path);
#else
    strncpy(path, comp->resourceLocation, MAX_PATH_LENGTH);
#endif

#if FMI_VERSION == 1
    strncat(path, "/resources/y.txt", MAX_PATH_LENGTH-strlen(path)-1);
#elif FMI_VERSION == 2
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

    return OK;
}


Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
    case vr_time:
        values[(*index)++] = comp->time;
        return OK;
    default:
        logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
        return Error;
    }
}


Status getInt32(ModelInstance* comp, ValueReference vr, int32_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_y:
            values[(*index)++] = M(y);
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
