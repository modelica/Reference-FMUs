#ifndef VARIABLESFILTERMODEL_H
#define VARIABLESFILTERMODEL_H

#include <QObject>
#include <QSortFilterProxyModel>

class VariablesFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    VariablesFilterModel();

protected:
    bool filterParameterVariables = true;
    bool filterInputVariables = true;
    bool filterOutputVariables = true;
    bool filterLocalVariables = true;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

public slots:
    void setFilterParamterVariables(bool filter);
    void setFilterInputVariables(bool filter);
    void setFilterOutputVariables(bool filter);
    void setFilterLocalVariables(bool filter);
};

#endif // VARIABLESFILTERMODEL_H
