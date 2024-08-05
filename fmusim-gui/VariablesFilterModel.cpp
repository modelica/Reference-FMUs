#include "VariablesFilterModel.h"
#include "FMIModelDescription.h"


VariablesFilterModel::VariablesFilterModel() {
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool VariablesFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {

    const QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

    const FMIModelVariable* variable = static_cast<FMIModelVariable*>(index.internalPointer());

    if (variable) {

        switch (variable->causality) {
        case FMIParameter:
        case FMIStructuralParameter:
            if (!filterParameterVariables) {
                return false;
            }
            break;
        case FMIInput:
            if (!filterInputVariables) {
                return false;
            }
            break;
        case FMIOutput:
            if (!filterOutputVariables) {
                return false;
            }
            break;
        default:
            if (!filterLocalVariables) {
                return false;
            }
            break;
        }

    }

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

void VariablesFilterModel::setFilterParamterVariables(bool filter) {
    filterParameterVariables = filter;
    invalidateRowsFilter();
}

void VariablesFilterModel::setFilterInputVariables(bool filter) {
    filterInputVariables = filter;
    invalidateRowsFilter();
}

void VariablesFilterModel::setFilterOutputVariables(bool filter) {
    filterOutputVariables = filter;
    invalidateRowsFilter();
}

void VariablesFilterModel::setFilterLocalVariables(bool filter) {
    filterLocalVariables = filter;
    invalidateRowsFilter();
}
