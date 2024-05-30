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
        case NAME_COLUMN_INDEX:
            return QIcon(":/icons/light/float_variable.svg");
        default:
            return QVariant();
        }
    case Qt::DisplayRole:
        switch (index.column()) {
        case NAME_COLUMN_INDEX:
            return variable->name;
        case TYPE_COLUMN_INDEX:
            switch (variable->type) {
                case FMIFloat32Type:
                case FMIDiscreteFloat32Type: return "Float32";
                case FMIFloat64Type:
                case FMIDiscreteFloat64Type: return "Float64";
                case FMIInt8Type: return "Int8";
                case FMIUInt8Type: return "UInt8";
                case FMIInt16Type: return "Int16";
                case FMIUInt16Type: return "UInt16";
                case FMIInt32Type: return "Int32";
                case FMIUInt32Type: return "UInt32";
                case FMIInt64Type: return "Int64";
                case FMIUInt64Type: return "UInt64";
                case FMIBooleanType: return "Boolean";
                case FMIStringType: return "String";
                case FMIBinaryType: return "Binary";
                case FMIClockType: return "Clock";
            }
        case DIMENSION_COLUMN_INDEX:
            if (variable->nDimensions > 0) {

                QString dimensions;

                for (size_t i = 0; i < variable->nDimensions; i++) {

                    FMIDimension* dimension = &variable->dimensions[i];

                    if (i > 0) {
                        dimensions += ", ";
                    }

                    if (dimension->start) {
                        dimensions += QString::number(dimension->start);
                    } else {
                        dimensions += dimension->variable->name;
                    }
                }
                return dimensions;
            }
            break;
        case VALUE_REFERENCE_COLUMN_INDEX:
            return variable->valueReference;
        case INITIAL_COLUMN_INDEX:
            switch(variable->initial) {
                case FMIUndefined: return "undefined";
                case FMIExact: return "exact";
                case FMIApprox: return "approx";
                case FMICalculated: return "calculated";
            }
        case CAUSALITY_COLUMN_INDEX:
            switch(variable->causality) {
                case FMIParameter: return "parameter";
                case FMICalculatedParameter: return "calculatedParameter";
                case FMIStructuralParameter: return "structuralParameter";
                case FMIInput: return "input";
                case FMIOutput: return "output";
                case FMILocal: return "local";
                case FMIIndependent: return "independent";
            }
        case VARIABITLITY_COLUMN_INDEX:
            switch(variable->variability) {
                case FMIConstant: return "Constant";
                case FMIFixed: return "Fixed";
                case FMITunable: return "Tunable";
                case FMIDiscrete: return "Discrete";
                case FMIContinuous: return "Continuous";
            }
        case START_COLUMN_INDEX:
            return variable->start;
        case NOMINAL_COLUMN_INDEX:
            return variable->nominal;
        case MIN_COLUMN_INDEX:
            return variable->min;
        case MAX_COLUMN_INDEX:
            return variable->max;
        case UNIT_COLUMN_INDEX:
            if (variable->declaredType && variable->declaredType->unit) {
                return variable->declaredType->unit->name;
            }
            break;
        case DESCRIPTION_COLUMN_INDEX:
            return variable->description;
        default: break;
        }
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case START_COLUMN_INDEX:
        case VALUE_REFERENCE_COLUMN_INDEX:
            return int(Qt::AlignRight | Qt::AlignVCenter);
        default: break;
        }
    default:
        break;
    }

    return QVariant();
}


QVariant ModelVariablesItemModel::headerData(int section, Qt::Orientation orientation, int role) const {

    Q_ASSERT(section < NUMBER_OF_COLUMNS);

    const char* columnNames[] = {"Name", "Type", "Dimensions", "Value Reference", "Initial", "Causality", "Variabitliy", "Start", "Nominal", "Min", "Max", "Unit", "Description"};

    switch (role) {
    case Qt::DisplayRole:
        return columnNames[section];
    }

    return QVariant();
}
