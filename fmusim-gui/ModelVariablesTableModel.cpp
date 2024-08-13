#include "ModelVariablesTableModel.h"
#include <QIcon>
#include <QFont>

ModelVariablesTableModel::ModelVariablesTableModel(QObject *parent)
    : AbstractModelVariablesModel{parent}
{
}

void ModelVariablesTableModel::setModelDescription(const FMIModelDescription* modelDescription) {
    beginResetModel();
    this->modelDescription = modelDescription;
    endResetModel();
}

void ModelVariablesTableModel::setStartValues(QMap<const FMIModelVariable*, QString> *startValues) {
    beginResetModel();
    this->startValues = startValues;
    endResetModel();
}

void ModelVariablesTableModel::setPlotVariables(QList<const FMIModelVariable*> *plotVariables) {
    beginResetModel();
    this->plotVariables = plotVariables;
    endResetModel();
}


QModelIndex ModelVariablesTableModel::index(int row, int column, const QModelIndex &parent) const {
    FMIModelVariable* variable = &modelDescription->modelVariables[row];
    return createIndex(row, column, variable);
}

QModelIndex ModelVariablesTableModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int ModelVariablesTableModel::rowCount(const QModelIndex &parent) const {

    if (!parent.isValid() && modelDescription) {
        return (int)modelDescription->nModelVariables;
    }

    return 0;
}

QVariant ModelVariablesTableModel::data(const QModelIndex &index, int role) const {

    const FMIModelVariable* variable = static_cast<FMIModelVariable*>(index.internalPointer());

    return columnData(variable, index.column(), role);
}

bool ModelVariablesTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {

    if (!index.isValid()) {
        return false;
    }

    const FMIModelVariable* variable = static_cast<FMIModelVariable*>(index.internalPointer());

    if (index.column() == START_COLUMN_INDEX) {

        if (value.toString().isEmpty()) {
            startValues->remove(variable);
        } else {
            startValues->insert(variable, value.toString());
        }

        return true;

    } else if (index.column() == PLOT_COLUMN_INDEX && role == Qt::CheckStateRole) {

        if (value == Qt::Checked) {
            emit plotVariableSelected(variable);
        } else {
            emit plotVariableDeselected(variable);
        }

        return true;
    }

    return false;
}
