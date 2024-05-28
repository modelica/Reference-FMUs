#ifndef MODELVARIABLESITEMMODEL_H
#define MODELVARIABLESITEMMODEL_H

#include <QAbstractItemModel>

extern "C" {
#include "FMIModelDescription.h"
}

class ModelVariablesItemModel : public QAbstractItemModel
{
public:
    const static int NUMBER_OF_COLUMNS = 3;

    const static int NAME_COLUMN_INDEX = 0;
    const static int START_COLUMN_INDEX = 1;
    const static int DESCRIPTION_COLUMN_INDEX = 2;

    const static int NAME_COLUMN_DEFAULT_WIDTH = 200;
    const static int START_COLUMN_DEFAULT_WIDTH = 70;

    explicit ModelVariablesItemModel(const FMIModelDescription* modelDescription, QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex &child) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

protected:
    const FMIModelDescription* modelDescription;
};

#endif // MODELVARIABLESITEMMODEL_H
