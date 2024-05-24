#include "ModelVariablesItemModel.h"

ModelVariablesItemModel::ModelVariablesItemModel(const FMIModelDescription* modelDescription, QObject *parent)
    : QAbstractItemModel{parent}
{
    this->modelDescription = modelDescription;
}

QModelIndex ModelVariablesItemModel::index(int row, int column, const QModelIndex &parent) const {
    FMIModelVariable* variable = &modelDescription->modelVariables[row];
    return createIndex(row, column, variable);
}

QModelIndex ModelVariablesItemModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int ModelVariablesItemModel::rowCount(const QModelIndex &parent) const {
    return (int)modelDescription->nModelVariables;
}

int ModelVariablesItemModel::columnCount(const QModelIndex &parent) const {
    return 1;
}

QVariant ModelVariablesItemModel::data(const QModelIndex &index, int role) const {

    FMIModelVariable* variable = static_cast<FMIModelVariable*>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        return variable->name;
    default:
        return QVariant();
    }
}
