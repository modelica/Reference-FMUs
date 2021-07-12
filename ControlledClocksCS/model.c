#include <math.h>  // for fabs()
#include "config.h"
#include "model.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int  global1_localcopy[5];
int  global2_localcopy[5];

int global1;
int global2;

#define M(v) (comp->modelData->v)

void setStartValues(ModelInstance *comp) {
    global1 = 0;
    global2 = 0;

}

void calculateValues(ModelInstance *comp) {
    UNUSED(comp)
}

void eventUpdate(ModelInstance *comp) {
    if (M(startTask1)==true)	global1_localcopy[0] = global1+1; // copy in and compute of Task1
    if (M(endTask1)==true)	M(output1) = global1 = global1_localcopy[0]; // copy out of Task1
    if (M(startTask2)==true)	global1_localcopy[1] = global1+5;   // copy in and compute of Task2
    if (M(endTask2)==true)	M(output1) = global1 = global1_localcopy[1];  // copy out of Task2  
    if (M(startTask3)==true)	global2_localcopy[2] = global2+1;    // copy in and compute of Task3
    if (M(endTask3)==true)	M(output2) = global2 = global2_localcopy[2];  // copy out of Task3    
    if (M(startTask4)==true)	global1_localcopy[3] = global1+3;   // copy in and compute of Task4
    if (M(endTask4)==true)	M(output1) = global1 = global1_localcopy[3]; // copy out of Task4 
    if (M(startTask5)==true)	global2_localcopy[4] = global2+3; // copy in and compute of Task5
    if (M(endTask5)==true)	M(output2) = global2 = global2_localcopy[4]; // copy out of Task5

    M(startTask1)=false;
    M(endTask1)=false;
    M(startTask2)=false;
    M(endTask2)=false;
    M(startTask3)=false;
    M(endTask3)=false;
    M(startTask4)=false;
    M(endTask4)=false;
    M(startTask5)=false;
    M(endTask5)=false;

    comp->valuesOfContinuousStatesChanged = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation  = false;
    comp->nextEventTimeDefined = false;
    comp->newDiscreteStatesNeeded = false;
}


Status setClock(ModelInstance* comp, ValueReference vr) {
    switch (vr) {
        case vr_startTask1:
            M(startTask1) = true;
            return OK;
        case vr_endTask1:
            M(endTask1) = true;
            return OK;
        case vr_startTask2:
            M(startTask2) = true;
            return OK;
        case vr_endTask2:
            M(endTask2) = true;
            return OK;
        case vr_startTask3:
            M(startTask3) = true;
            return OK;
        case vr_endTask3:
            M(endTask3) = true;
            return OK;
        case vr_startTask4:
            M(startTask4) = true;
            return OK;
        case vr_endTask4:
            M(endTask4) = true;
            return OK;
        case vr_startTask5:
            M(startTask5) = true;
            return OK;
        case vr_endTask5:
            M(endTask5) = true;
            return OK;
        default:
            return Error;
    }

    return OK;
}


Status getInt32(ModelInstance* comp, ValueReference vr, int32_t *value, size_t *index){

    switch (vr) {
    case vr_output1:
        value[(*index)++] = M(output1);
        return OK;
    case vr_output2:
        value[(*index)++] = M(output2);
        return OK;
    default:
        return Error;
    }

}
