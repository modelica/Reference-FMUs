#include "SimulationThread.h"
#include <Windows.h>

SimulationThread::SimulationThread() {}

void SimulationThread::run()
{
    for (int i = 1; i <= 100; i++) {
        Sleep(20);
        emit progressChanged(i);
    }

    // status = FMISimulate(S, modelDescripton, unzipdir, recorder, input, settings);
}
