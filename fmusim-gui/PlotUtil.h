#ifndef PLOTUTIL_H
#define PLOTUTIL_H

#include <QString>

extern "C" {
typedef struct FMIRecorder FMIRecorder;
typedef struct FMIModelVariable FMIModelVariable;
}

class PlotUtil
{
private:
    PlotUtil();

public:
    static QString createPlot(const FMIRecorder* initialRecorder, const FMIRecorder* recorder, const QList<const FMIModelVariable*> plotVariables, Qt::ColorScheme colorScheme);
};

#endif // PLOTUTIL_H
