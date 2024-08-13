#ifndef MODELVARIABLESTABLEMODEL_H
#define MODELVARIABLESTABLEMODEL_H

#include "AbstractModelVariablesModel.h"


class ModelVariablesTableModel : public AbstractModelVariablesModel
{
    Q_OBJECT

public:
    explicit ModelVariablesTableModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex &child) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

protected:
    const FMIModelVariable* variableForIndex(const QModelIndex &index) const override;

};

#endif // MODELVARIABLESTABLEMODEL_H
