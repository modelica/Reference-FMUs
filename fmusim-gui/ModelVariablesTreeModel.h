#ifndef MODELVARIABLESTREEMODEL_H
#define MODELVARIABLESTREEMODEL_H

#include <QAbstractItemModel>
#include "FMIModelDescription.h"


class TreeItem {

public:
    TreeItem* parent = nullptr;
    QString name;
    const FMIModelVariable* modelVariable;
    QList<TreeItem*> children;
    TreeItem* find(const QString& name) const;

    void addChild(TreeItem* treeItem);
};

class ModelVariablesTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    TreeItem* rootItem = nullptr;

    explicit ModelVariablesTreeModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setModelDescription(const FMIModelDescription* modelDescription);

private:
};

#endif // MODELVARIABLESTREEMODEL_H
