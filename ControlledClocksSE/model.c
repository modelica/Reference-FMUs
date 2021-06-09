#include "config.h"
#include "model.h"
#include <math.h>
#include <stdlib.h>

#define TASK1 0
#define TASK2 1
#define TASK3 2
#define TASK4 3
#define TASK5 4

int global1;
int global2;
int global1_localcopy[5];
int global2_localcopy[5];


static void activateModelPartitionTask1Start(ModelInstance *comp, double time) {
    global1_localcopy[TASK1] = global1;  // copy-in
    global1_localcopy[TASK1] = global1_localcopy[TASK1] + 1;  // compute
}

static void activateModelPartitionTask1End(ModelInstance* comp, double time) {
    M(output1) = global1 = global1_localcopy[TASK1];  // copy-out
}

 static void activateModelPartitionTask2Start(ModelInstance *comp, double time) {
	global1_localcopy[TASK2] = global1;  // copy-in
	global1_localcopy[TASK2] = global1_localcopy[TASK2] + 5;  // compute
}

static void activateModelPartitionTask2End(ModelInstance *comp, double time) {
	M(output1) = global1 = global1_localcopy[TASK2];  // copy-out
}

static void activateModelPartitionTask3Start(ModelInstance *comp, double time) {
	global2_localcopy[TASK3] = global2;  // copy-in
	global2_localcopy[TASK3] = global2_localcopy[TASK3] + 1;  // compute
}

static void activateModelPartitionTask3End(ModelInstance *comp, double time) {
	M(output2) = global2 = global2_localcopy[TASK3];  // copy-out
}

static void activateModelPartitionTask4Start(ModelInstance *comp, double time) {
	global1_localcopy[TASK4] = global1;  // copy-in
	global1_localcopy[TASK4] = global1_localcopy[TASK4] + 3;  // compute
}

static void activateModelPartitionTask4End(ModelInstance *comp, double time) {
    M(output1) = global1 = global1_localcopy[TASK4];  // copy-out
}

static void activateModelPartitionTask5Start(ModelInstance *comp, double time) {
    global2_localcopy[TASK5] = global2;  // copy-in
    global2_localcopy[TASK5] = global2_localcopy[TASK5] + 3;  // compute
}

static void activateModelPartitionTask5End(ModelInstance *comp, double time) {
    M(output2) = global2 = global2_localcopy[TASK5];  // copy-out
}

void setStartValues(ModelInstance *comp) {
	global1 = 0;
	global2 = 0;
}

void calculateValues(ModelInstance *comp) {
    // nothing to do
}

void eventUpdate(ModelInstance *comp) {
    // nothing to do
}

Status activateClock(ModelInstance* comp, ValueReference vr) {
    return OK;
}

Status getInt32(ModelInstance* comp, ValueReference vr, int *value, size_t *index) {
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

Status getClock(ModelInstance* comp, ValueReference vr, int* value) {
	return OK;
}

Status activateModelPartition(ModelInstance* comp, ValueReference vr, double activationTime) {
    switch (vr) {
        case vr_startTask1:
            activateModelPartitionTask1Start(comp, activationTime);
            return OK;
        case vr_endTask1:
            activateModelPartitionTask1End(comp, activationTime);
            return OK;
        case vr_startTask2:
            activateModelPartitionTask2Start(comp, activationTime);
            return OK;
		case vr_endTask2:
            activateModelPartitionTask2End(comp, activationTime);
            return OK;
        case vr_startTask3:
            activateModelPartitionTask3Start(comp, activationTime);
            return OK;
        case vr_endTask3:
            activateModelPartitionTask3End(comp, activationTime);
            return OK;
		case vr_startTask4:
            activateModelPartitionTask4Start(comp, activationTime);
            return OK;
        case vr_endTask4:
            activateModelPartitionTask4End(comp, activationTime);
            return OK;
        case vr_startTask5:
            activateModelPartitionTask5Start(comp, activationTime);
            return OK;
		case vr_endTask5:
            activateModelPartitionTask5End(comp, activationTime);
            return OK;
        default:
            return Error;
    }
}
