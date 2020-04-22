#ifndef callbacks_h
#define callbacks_h

static void cb_logMessage(fmi3InstanceEnvironment instanceEnvironment, fmi3String instanceName, fmi3Status status, fmi3String category, fmi3String message) {
    
    char *prefix;
    
    switch (status) {
        case fmi3OK:
            prefix = "OK";
            break;
        case fmi3Warning:
            prefix = "Warning";
            break;
        case fmi3Discard:
            prefix = "Discard";
            break;
        case fmi3Error:
            prefix = "Error";
            break;
        case fmi3Fatal:
            prefix = "Fatal";
            break;
    }
    
    printf("\n[%s] %s", prefix, message);
}

#endif /* callbacks_h */
