#include <QList>
#include "FMIRecorder.h"
#include "PlotUtil.h"


PlotUtil::PlotUtil() {}

static QString getData(const FMIRecorder* recorder, const FMIModelVariable* variable) {

    const FMIVariableType type = variable->type;

    bool found = false;

    size_t index;

    for (index = 0; index < recorder->variableInfos[type]->nVariables; index++) {
        if (recorder->variableInfos[type]->variables[index] == variable) {
            found = true;
            break;
        }
    }

    if (!found) {
        return "";
    }

    QString y;

    for (size_t j = 0; j < recorder->nRows; j++) {

        Row* row = recorder->rows[j];

        if (j > 0) {
            y += ", ";
        }

        switch (type) {
        case FMIFloat32Type:
        case FMIDiscreteFloat32Type:
        {
            const float* values = (float*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        case FMIFloat64Type:
        case FMIDiscreteFloat64Type:
        {
            const double* values = (double*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        case FMIInt8Type:
        {
            const int8_t* values = (int8_t*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        case FMIUInt8Type:
        {
            const uint8_t* values = (uint8_t*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        case FMIInt16Type:
        {
            const int16_t* values = (int16_t*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        case FMIUInt16Type:
        {
            const uint16_t* values = (uint16_t*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        case FMIInt32Type:
        {
            const int32_t* values = (int32_t*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        case FMIUInt32Type:
        {
            const uint32_t* values = (uint32_t*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        case FMIInt64Type:
        {
            const int64_t* values = (int64_t*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        case FMIUInt64Type:
        {
            const uint64_t* values = (uint64_t*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        case FMIBooleanType:
        {
            const bool* values = (bool*)row->values[type];
            y += QString::number(values[index]);
        }
        break;
        default:
            y += "0";
            break;
        }

    }

    return y;
}


QString PlotUtil::createPlot(const FMIRecorder* initialRecorder, const FMIRecorder* recorder, const QList<const FMIModelVariable*> plotVariables, Qt::ColorScheme colorScheme) {

    QString data;

    data += "var time = [";

    for (size_t i = 0; i < recorder->nRows; i++) {

        Row* row = recorder->rows[i];

        if (i > 0) {
            data += ", ";
        }

        data += QString::number(row->time);
    }

    data += "];\n";

    data += "var time2 = [";

    data += QString::number(recorder->rows[0]->time);

    data += ", ";

    data += QString::number(recorder->rows[recorder->nRows - 1]->time);

    data += "];\n";

    data += "var data = [\n";

    size_t k = 0;

    for (const FMIModelVariable* variable : plotVariables) {

        if (k > 0) {
            data += ", ";
        }

        QString y = getData(recorder, variable);

        if (y.isEmpty()) {
            y = getData(initialRecorder, variable);
            y = y + ", " + y;
            data += "{x: time2, y: [" + y + "], ";
        } else {
            data += "{x: time, y: [" + y + "], ";
        }

        data += "type: 'scatter', type: 'line', mode: 'lines', name: '', line: {color: '#248BD2', width: 1.5";

        if (variable->variability == FMIContinuous) {
            data += "}";
        } else if (variable->type == FMIBooleanType) {
            data += ", shape: 'hv'}, fill: 'tozeroy'";
        } else {
            data += ", shape: 'hv'}";
        }

        if (k > 0) {
            const QString plotIndex = QString::number(k + 1);
            data += ", xaxis: 'x" + plotIndex + "', yaxis: 'y" + plotIndex + "'";
        }

        data += "}\n";

        k++;
    }

    data += "];\n";

    QString axes;

    k = 0;

    for (const FMIModelVariable* variable : plotVariables) {

        const QString name = QString::fromUtf8(variable->name);

        const QString colors = colorScheme == Qt::ColorScheme::Dark ? "color: '#fff', zerolinecolor: '#666'" : "color: '#000', zerolinecolor: '#000'";

        const double segment = 1.0 / plotVariables.size();
        const double margin = 0.02;

        const QString domain = "[" + QString::number(k * segment + (k == 0 ? 0.0 : margin)) + ", " + QString::number((k + 1) * segment - (k == recorder->nVariables ? 0.0 : margin)) + "]";

        if (k == 0) {
            axes += "xaxis: {" + colors + "},";
            axes += "yaxis: {title: '" + name + "', titlefont: {size: 13}, " + colors + ", domain: " + domain + "},";
        } else {
            axes += "xaxis" + QString::number(k + 1) +  ": {" + colors + ", matches: 'x'},";
            axes += "yaxis" + QString::number(k + 1) +  ": {title: '" + name + "', titlefont: {size: 13}, " + colors + ", domain: " + domain + "},";
        }

        k++;
    }

    QString plotColors = colorScheme == Qt::ColorScheme::Dark ? "plot_bgcolor: '#1e1e1e', paper_bgcolor: '#1e1e1e'," : "";

    QString javaScript =
        data +
        "    var layout = {"
        "    showlegend: false,"
        "    autosize: true,"
        "    font: {family: 'Segoe UI', size: 12},"
        + plotColors +
        "    grid: {rows: " + QString::number(plotVariables.size()) + ", columns: 1, pattern: 'independent'},"
        + axes +
        "    margin: { l: 60, r: 20, b: 20, t: 20, pad: 0 }"
        "};"
        "var config = {"
        "    'responsive': true"
        "};"
        "Plotly.newPlot('gd', data, layout, config);";

    return javaScript;
}
