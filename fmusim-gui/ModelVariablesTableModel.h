#ifndef MODELVARIABLESTABLEMODEL_H
#define MODELVARIABLESTABLEMODEL_H

#include "AbstractModelVariablesModel.h"

extern "C" {
#include "FMIModelDescription.h"
}

class ModelVariablesTableModel : public AbstractModelVariablesModel
{
    Q_OBJECT

public:
    explicit ModelVariablesTableModel(QObject *parent = nullptr);

    void setModelDescription(const FMIModelDescription* modelDescription);

    void setStartValues(QMap<const FMIModelVariable*, QString> *startValues);

    void setPlotVariables(QList<const FMIModelVariable*> *plotVariables);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex &child) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

signals:
    void plotVariableSelected(const FMIModelVariable* variable);
    void plotVariableDeselected(const FMIModelVariable* variable);

};

#endif // MODELVARIABLESTABLEMODEL_H
