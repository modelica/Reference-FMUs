/* This example demonstrates how to import an FMU implemented as a static library or source code*/

// FMI function prefix (from XML)
#define FMI3_FUNCTION_PREFIX VanDerPol_
#include "fmi3Functions.h"
#undef FMI3_FUNCTION_PREFIX

#define INSTANTIATION_TOKEN "{8c4e810f-3da3-4a00-8276-176fa3c9f000}"


static void cb_logMessage(fmi3InstanceEnvironment instanceEnvironment, fmi3String instanceName, fmi3Status status, fmi3String category, fmi3String message) {
    // log message
}

int main(int argc, char* argv[]) {

    fmi3Instance m = VanDerPol_fmi3InstantiateModelExchange(
        "instance1",             // instance name
        INSTANTIATION_TOKEN,     // instantiation token (from XML)
        "file:///tmp/VanDerPol", // resource location (extracted FMU)
        fmi3False,               // visible
        fmi3False,               // debug logging disabled
        NULL,                    // instance environment
        cb_logMessage);          // logger callback

    // simulation ...

    VanDerPol_fmi3FreeInstance(m);

    return m ? EXIT_SUCCESS : EXIT_FAILURE;
}
