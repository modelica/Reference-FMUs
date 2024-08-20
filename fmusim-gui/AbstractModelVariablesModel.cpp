#include <QIcon>
#include <QFont>
#include "AbstractModelVariablesModel.h"


AbstractModelVariablesModel::AbstractModelVariablesModel(QObject *parent)
    : QAbstractItemModel{parent}
{
}

void AbstractModelVariablesModel::setModelDescription(const FMIModelDescription* modelDescription) {
    beginResetModel();
    this->modelDescription = modelDescription;
    endResetModel();
}

void AbstractModelVariablesModel::setStartValues(QMap<const FMIModelVariable*, QString> *startValues) {
    beginResetModel();
    this->startValues = startValues;
    endResetModel();
}

void AbstractModelVariablesModel::setPlotVariables(QList<const FMIModelVariable*> *plotVariables) {
    beginResetModel();
    this->plotVariables = plotVariables;
    endResetModel();
}

int AbstractModelVariablesModel::columnCount(const QModelIndex &parent) const {
    return NumberOfColumns;
}

QVariant AbstractModelVariablesModel::headerData(int section, Qt::Orientation orientation, int role) const {

    Q_ASSERT(section < NumberOfColumns);

    if (role == Qt::DisplayRole) {

        switch (section) {
        case NameColumn:
            return "Name";
        case TypeColumn:
            return "Type";
        case DimensionColumn:
            return "Dimensions";
        case ValueReferenceColumn:
            return "Value Reference";
        case InitialColumn:
            return "Initial";
        case CausalityColumn:
            return "Causality";
        case VariabilityColumn:
            return "Variabitliy";
        case StartColumn:
            return "Start";
        case NominalColumn:
            return "Nominal";
        case MinColumn:
            return "Min";
        case MaxColumn:
            return "Max";
        case UnitColumn:
            return "Unit";
        case PlotColumn:
            return "Plot";
        case DescriptionColumn:
            return "Description";
        default:
            break;
        }
    }

    return QVariant();
}

bool AbstractModelVariablesModel::setData(const QModelIndex &index, const QVariant &value, int role) {

    if (!index.isValid()) {
        return false;
    }

    const FMIModelVariable* variable = variableForIndex(index);

    if (index.column() == StartColumn) {

        if (value.toString().isEmpty()) {
            startValues->remove(variable);
        } else {
            startValues->insert(variable, value.toString());
        }

        return true;

    } else if (index.column() == PlotColumn && role == Qt::CheckStateRole) {

        if (value == Qt::Checked) {
            emit plotVariableSelected(variable);
        } else {
            emit plotVariableDeselected(variable);
        }

        return true;
    }

    return false;
}

Qt::ItemFlags AbstractModelVariablesModel::flags(const QModelIndex &index) const {

    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    if (index.column() == StartColumn && startValues) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    } else if (index.column() == PlotColumn) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    } else {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
}

QVariant AbstractModelVariablesModel::columnData(const FMIModelVariable *variable, int column, int role) const {

    const bool isFMI3 = modelDescription->fmiMajorVersion == FMIMajorVersion3;

    switch (role) {
    case Qt::DecorationRole:
        switch (column) {
        case NameColumn:
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
        case NameColumn:
            return variable->name;
        case TypeColumn:
            switch (variable->type) {
            case FMIFloat32Type:
                return "Float32";
            case FMIFloat64Type:
                return isFMI3 ? "Float64" : "Real";
            case FMIInt8Type:
                return "Int8";
            case FMIUInt8Type:
                return "UInt8";
            case FMIInt16Type:
                return "Int16";
            case FMIUInt16Type:
                return "UInt16";
            case FMIInt32Type:
                return isFMI3 ? "Int32" : "Integer";
            case FMIUInt32Type:
                return "UInt32";
            case FMIInt64Type:
                return "Int64";
            case FMIUInt64Type:
                return "UInt64";
            case FMIBooleanType:
                return "Boolean";
            case FMIStringType:
                return "String";
            case FMIBinaryType:
                return "Binary";
            case FMIClockType:
                return "Clock";
            }
        case DimensionColumn:
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
        case ValueReferenceColumn:
            return variable->valueReference;
        case InitialColumn:
            switch(variable->initial) {
            case FMIUndefined: return "undefined";
            case FMIExact: return "exact";
            case FMIApprox: return "approx";
            case FMICalculated: return "calculated";
            }
        case CausalityColumn:
            switch(variable->causality) {
            case FMIParameter: return "parameter";
            case FMICalculatedParameter: return "calculatedParameter";
            case FMIStructuralParameter: return "structuralParameter";
            case FMIInput: return "input";
            case FMIOutput: return "output";
            case FMILocal: return "local";
            case FMIIndependent: return "independent";
            }
        case VariabilityColumn:
            switch(variable->variability) {
            case FMIConstant: return "constant";
            case FMIFixed: return "fixed";
            case FMITunable: return "tunable";
            case FMIDiscrete: return "discrete";
            case FMIContinuous: return "continuous";
            }
        case StartColumn:
            if (startValues && startValues->contains(variable)) {
                return startValues->value(variable);
            } else {
                return variable->start;
            }
        case NominalColumn:
            return variable->nominal;
        case MinColumn:
            return variable->min;
        case MaxColumn:
            return variable->max;
        case UnitColumn:
            if (variable->declaredType && variable->declaredType->unit) {
                return variable->declaredType->unit->name;
            }
            break;
        case DescriptionColumn:
            return variable->description;
        default: break;
        }
    case Qt::TextAlignmentRole:
        switch (column) {
        case StartColumn:
        case ValueReferenceColumn:
            return int(Qt::AlignRight | Qt::AlignVCenter);
        default: break;
        }
    case Qt::FontRole:
        if (column == StartColumn && startValues && startValues->contains(variable)) {
            QFont font;
            font.setBold(true);
            return font;
        }
        break;
    case Qt::CheckStateRole:
        if (column == PlotColumn) {
            return (plotVariables && plotVariables->contains(variable)) ? Qt::Checked : Qt::Unchecked;
        }
        break;
    default:
        break;
    }

    return QVariant();
}
