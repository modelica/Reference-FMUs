#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#define FMI3_FUNCTION_PREFIX plant_
#include "fmi3Functions.h"
#undef FMI3_FUNCTION_PREFIX

#include "util.h"

#define FIXED_STEP 1e-2
#define STOP_TIME 3
#define OUTPUT_FILE_HEADER "time,h,v\n"

int main(int argc, char *argv[])
{
    printf("Running model_exchange example... \n");
    


    printf("Sucess! \n");
    return EXIT_SUCCESS;
}
