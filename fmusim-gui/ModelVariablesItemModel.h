#ifndef MODELVARIABLESITEMMODEL_H
#define MODELVARIABLESITEMMODEL_H

#include <QAbstractItemModel>

extern "C" {
#include "FMIModelDescription.h"
}

class ModelVariablesItemModel : public QAbstractItemModel
{
public:
    explicit ModelVariablesItemModel(const FMIModelDescription* modelDescription, QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex &child) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

protected:
    const FMIModelDescription* modelDescription;
};

#endif // MODELVARIABLESITEMMODEL_H
