#include <QIcon>
#include <QFont>
#include "AbstractModelVariablesModel.h"


AbstractModelVariablesModel::AbstractModelVariablesModel(QObject *parent)
    : QAbstractItemModel{parent}
{
}

int AbstractModelVariablesModel::columnCount(const QModelIndex &parent) const
{
    return NUMBER_OF_COLUMNS;
}

QVariant AbstractModelVariablesModel::headerData(int section, Qt::Orientation orientation, int role) const {

    Q_ASSERT(section < NUMBER_OF_COLUMNS);

    const char* columnNames[] = {"Name", "Type", "Dimensions", "Value Reference", "Initial", "Causality", "Variabitliy", "Start", "Nominal", "Min", "Max", "Unit", "Plot", "Description"};

    switch (role) {
    case Qt::DisplayRole:
        return columnNames[section];
    }

    return QVariant();
}

Qt::ItemFlags AbstractModelVariablesModel::flags(const QModelIndex &index) const {

    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    if (index.column() == START_COLUMN_INDEX && startValues) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    } else if (index.column() == PLOT_COLUMN_INDEX) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    } else {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
}

QVariant AbstractModelVariablesModel::columnData(const FMIModelVariable *variable, int column, int role) const {

    switch (role) {
    case Qt::DecorationRole:
        switch (column) {
        case NAME_COLUMN_INDEX:
            switch (variable->causality) {
            case FMIParameter:
            case FMIStructuralParameter:
            case FMICalculatedParameter:
                return QIcon(":/variables/dark/float-parameter.svg");
            case FMIInput:
                return QIcon(":/variables/dark/float-input.svg");
            case FMIOutput:
                return QIcon(":/variables/dark/float-output.svg");
            default:
                return QIcon(":/variables/dark/float-variable.svg");
            }
        default:
            return QVariant();
        }
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (column) {
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
            case FMIConstant: return "constant";
            case FMIFixed: return "fixed";
            case FMITunable: return "tunable";
            case FMIDiscrete: return "discrete";
            case FMIContinuous: return "continuous";
            }
        case START_COLUMN_INDEX:
            if (startValues && startValues->contains(variable)) {
                return startValues->value(variable);
            } else {
                return variable->start;
            }
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
        switch (column) {
        case START_COLUMN_INDEX:
        case VALUE_REFERENCE_COLUMN_INDEX:
            return int(Qt::AlignRight | Qt::AlignVCenter);
        default: break;
        }
    case Qt::FontRole:
        if (column == START_COLUMN_INDEX && startValues && startValues->contains(variable)) {
            QFont font;
            font.setBold(true);
            return font;
        }
        break;
    case Qt::CheckStateRole:
        if (column == PLOT_COLUMN_INDEX) {
            return (plotVariables && plotVariables->contains(variable)) ? Qt::Checked : Qt::Unchecked;
        }
        break;
    default:
        break;
    }

    return QVariant();
}
