#include "ModelVariablesTableModel.h"
#include <QIcon>
#include <QFont>

ModelVariablesTableModel::ModelVariablesTableModel(QObject *parent)
    : AbstractModelVariablesModel{parent}
{
}

QModelIndex ModelVariablesTableModel::index(int row, int column, const QModelIndex &parent) const {

    FMIModelVariable* variable = nullptr;

    if (row >= 0 && row < modelDescription->nModelVariables) {
        variable = modelDescription->modelVariables[row];
    }

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

const FMIModelVariable *ModelVariablesTableModel::variableForIndex(const QModelIndex &index) const {
    return static_cast<FMIModelVariable*>(index.internalPointer());
}
