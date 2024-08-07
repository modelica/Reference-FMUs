#ifndef MODELVARIABLESITEMMODEL_H
#define MODELVARIABLESITEMMODEL_H

#include <QAbstractItemModel>

extern "C" {
#include "FMIModelDescription.h"
}

class ModelVariablesItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    const static int NUMBER_OF_COLUMNS = 14;

    const static int NAME_COLUMN_INDEX = 0;
    const static int TYPE_COLUMN_INDEX = 1;
    const static int DIMENSION_COLUMN_INDEX = 2;
    const static int VALUE_REFERENCE_COLUMN_INDEX = 3;
    const static int INITIAL_COLUMN_INDEX = 4;
    const static int CAUSALITY_COLUMN_INDEX = 5;
    const static int VARIABITLITY_COLUMN_INDEX = 6;
    const static int START_COLUMN_INDEX = 7;
    const static int NOMINAL_COLUMN_INDEX = 8;
    const static int MIN_COLUMN_INDEX = 9;
    const static int MAX_COLUMN_INDEX = 10;
    const static int UNIT_COLUMN_INDEX = 11;
    const static int PLOT_COLUMN_INDEX = 12;
    const static int DESCRIPTION_COLUMN_INDEX = 13;

    const static int NAME_COLUMN_DEFAULT_WIDTH = 200;
    const static int START_COLUMN_DEFAULT_WIDTH = 70;

    explicit ModelVariablesItemModel(QObject *parent = nullptr);

    void setModelDescription(const FMIModelDescription* modelDescription);

    void setStartValues(QMap<const FMIModelVariable*, QString> *startValues);

    void setPlotVariables(QList<const FMIModelVariable*> *plotVariables);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex &child) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

signals:
    void plotVariableSelected(const FMIModelVariable* variable);
    void plotVariableDeselected(const FMIModelVariable* variable);

protected:
    const FMIModelDescription* modelDescription = nullptr;
    QMap<const FMIModelVariable*, QString> *startValues = nullptr;
    QList<const FMIModelVariable*> *plotVariables = nullptr;

};

#endif // MODELVARIABLESITEMMODEL_H
