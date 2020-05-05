#ifndef util_h
#define util_h


#ifndef min
#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#endif

#ifndef max
#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

// tag::CheckStatus[]
#define CHECK_STATUS(S) status = S; if (status != fmi3OK) goto TERMINATE;
// end::CheckStatus[]

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

#endif /* util_h */
