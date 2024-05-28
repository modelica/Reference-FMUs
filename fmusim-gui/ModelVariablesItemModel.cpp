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
    return NUMBER_OF_COLUMNS;
}

QVariant ModelVariablesItemModel::data(const QModelIndex &index, int role) const {

    const FMIModelVariable* variable = static_cast<FMIModelVariable*>(index.internalPointer());

    switch (role) {
    case Qt::DecorationRole:
        switch (index.column()) {
        case NAME_COLUMN_INDEX: return QIcon(":/icons/light/float_variable.svg");
        default: break;
        }
    case Qt::DisplayRole:
        switch (index.column()) {
        case NAME_COLUMN_INDEX: return variable->name;
        case START_COLUMN_INDEX: return variable->start;
        case DESCRIPTION_COLUMN_INDEX: return variable->description;
        default: break;
        }
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case START_COLUMN_INDEX: return int(Qt::AlignRight | Qt::AlignVCenter);
        default: break;
        }
    default:
        break;
    }

    return QVariant();
}


QVariant ModelVariablesItemModel::headerData(int section, Qt::Orientation orientation, int role) const {

    Q_ASSERT(section < NUMBER_OF_COLUMNS);

    const char* columnNames[] = {"Name", "Start", "Description"};

    switch (role) {
    case Qt::DisplayRole:
        return columnNames[section];
    }

    return QVariant();
}
