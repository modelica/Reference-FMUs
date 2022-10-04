/* This example demonstrates how to import an FMU implemented as a shared library */

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include <stdlib.h>

// FMI function types
#include "fmi3FunctionTypes.h"

#define INSTANTIATION_TOKEN "{BD403596-3166-4232-ABC2-132BDF73E644}"


static void cb_logMessage(fmi3InstanceEnvironment instanceEnvironment, fmi3Status status, fmi3String category, fmi3String message) {
    // log message...
}

int main(int argc, char* argv[]) {

#if defined(_WIN32)
    HMODULE libraryHandle = LoadLibraryA("VanDerPol\\binaries\\x86_64-windows\\VanDerPol.dll");
#elif defined(__APPLE__)
    void *libraryHandle = dlopen("VanDerPol/binaries/x86_64-darwin/VanDerPol.dylib", RTLD_LAZY);
#else
    void *libraryHandle = dlopen("VanDerPol/binaries/x86_64-linux/VanDerPol.so", RTLD_LAZY);
#endif

    if (!libraryHandle) {
        return EXIT_FAILURE;
    }

    fmi3InstantiateModelExchangeTYPE *instantiateModelExchange = (fmi3InstantiateModelExchangeTYPE *)
#ifdef _WIN32
        GetProcAddress(libraryHandle, "fmi3InstantiateModelExchange");
#else
        dlsym(libraryHandle, "fmi3InstantiateModelExchange");
#endif

    fmi3FreeInstanceTYPE *freeInstance = (fmi3FreeInstanceTYPE *)
#ifdef _WIN32
        GetProcAddress(libraryHandle, "fmi3FreeInstance");
#else
        dlsym(libraryHandle, "fmi3FreeInstance");
#endif

    // load remaining FMI functions...

    if (!instantiateModelExchange || !freeInstance) {
        return EXIT_FAILURE;
    }

    fmi3Instance m = instantiateModelExchange(
        "instance1",         // instance name
        INSTANTIATION_TOKEN, // instantiation token (from XML)
        NULL,                // resource path
        fmi3False,           // visible
        fmi3False,           // debug logging disabled
        NULL,                // instance environment
        cb_logMessage);      // logger callback


    if (!m) {
        return EXIT_FAILURE;
    }

    // simulation...

    freeInstance(m);

    // unload shared library
#ifdef _WIN32
    FreeLibrary(libraryHandle);
#else
    dlclose(libraryHandle);
#endif

    return EXIT_SUCCESS;
}
