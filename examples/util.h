#ifndef util_h
#define util_h

#ifndef min
#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#endif

#ifndef max
#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

static void cb_logMessage(fmi3InstanceEnvironment instanceEnvironment, fmi3String instanceName, fmi3Status status, fmi3String category, fmi3String message) {
    
    switch (status) {
        case fmi3OK:
            printf("[OK] ");
            break;
        case fmi3Warning:
            printf("[Warning] ");
            break;
        case fmi3Discard:
            printf("[Discard] ");
            break;
        case fmi3Error:
            printf("[Error] ");
            break;
        case fmi3Fatal:
            printf("[Fatal] ");
            break;
    }

    puts(message);
}

static void* cb_allocateMemory(fmi3InstanceEnvironment instanceEnvironment, size_t nobj, size_t size) {
    return calloc(nobj, size);
}

static void cb_freeMemory(fmi3InstanceEnvironment instanceEnvironment, void* obj)  {
    free(obj);
}

//static void cb_lockPreemption() {
//    // do nothing
//}
//
//static void cb_unlockPreemption() {
//    // do nothing
//}

#endif /* util_h */
