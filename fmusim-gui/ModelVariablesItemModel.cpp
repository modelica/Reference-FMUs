#include "ModelVariablesItemModel.h"
#include <QIcon>

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

    if (!parent.isValid()) {
        return (int)modelDescription->nModelVariables;
    } else {
        return 0;
    }
}

int ModelVariablesItemModel::columnCount(const QModelIndex &parent) const {
    return 2;
}

QVariant ModelVariablesItemModel::data(const QModelIndex &index, int role) const {

    const FMIModelVariable* variable = static_cast<FMIModelVariable*>(index.internalPointer());

    switch (role) {
    case Qt::DecorationRole:
        if (index.column() == 0) return QIcon(":/icons/light/float_variable.svg");
        break;
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return variable->name;
        case 1:
            return variable->description;
        default:
            return "?";
        }
    default:
        break;
    }

    return QVariant();
}


QVariant ModelVariablesItemModel::headerData(int section, Qt::Orientation orientation, int role) const {

    const char* columnNames[] = {"Name", "Description"};

    switch (role) {
    case Qt::DisplayRole:
        return columnNames[section];
    }

    return QVariant();
}
