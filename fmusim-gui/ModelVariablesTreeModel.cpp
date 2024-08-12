#include <QIcon>
#include "ModelVariablesTreeModel.h"


ModelVariablesTreeModel::ModelVariablesTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TreeItem();
    rootItem->name = "root";
}

QVariant ModelVariablesTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {

    if (role == Qt::DisplayRole) {
        return "Name";
    }

    return QVariant();
}

QModelIndex ModelVariablesTreeModel::index(int row, int column, const QModelIndex &parent) const {

    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    const TreeItem* parentItem = parent.isValid() ? (TreeItem*)parent.internalPointer() : rootItem;

    const TreeItem* childItem = parentItem->children[row];

    return createIndex(row, column, childItem);
}

QModelIndex ModelVariablesTreeModel::parent(const QModelIndex &index) const {

    if (!index.isValid()) {
        return QModelIndex();
    }

    const TreeItem* childItem = (TreeItem*)index.internalPointer();
    const TreeItem* parentItem = childItem->parent;

    if (parentItem == rootItem) {
        return QModelIndex();
    }

    return createIndex(index.row(), 0, parentItem);
}

int ModelVariablesTreeModel::rowCount(const QModelIndex &parent) const {

    TreeItem* parentItem = parent.isValid() ? (TreeItem*)parent.internalPointer() : rootItem;

    return parentItem->children.length();
}

int ModelVariablesTreeModel::columnCount(const QModelIndex &parent) const {

    // if (!parent.isValid())
    //     return 0;

    return 1;
}

QVariant ModelVariablesTreeModel::data(const QModelIndex &index, int role) const {

    if (!index.isValid())
        return QVariant();

    const TreeItem* treeItem = (TreeItem*)index.internalPointer();

    if (role == Qt::DisplayRole) {
        return treeItem->name;
    }

    if (role == Qt::DecorationRole) {
        return QIcon(":/variables/dark/float-variable.svg");
    }

    return QVariant();
}

void ModelVariablesTreeModel::setModelDescription(const FMIModelDescription *modelDescription) {

    beginResetModel();

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        const FMIModelVariable* variable = &modelDescription->modelVariables[i];

        QString name = variable->name;
        QString prefix;
        QString suffix;

        if (name.startsWith("der(") && name.endsWith(")")) {
            prefix = "der(";
            suffix = ")";
            name = name.sliced(4, name.length() - 5);
        }

        const QStringList segments = QString(name).split('.');

        TreeItem* parentItem = rootItem;

        for (const QString& segment : segments.sliced(0, segments.length() - 1)) {

            TreeItem* p = parentItem->find(segment);

            if (!p) {
                p = new TreeItem();
                p->name = segment;
                parentItem->addChild(p);
            }

            parentItem = p;
        }

        TreeItem* treeItem = new TreeItem();

        treeItem->name = prefix + segments.last() + suffix;
        treeItem->modelVariable = variable;

        parentItem->addChild(treeItem);
    }

    endResetModel();
}

TreeItem *TreeItem::find(const QString &name) const {

    for (TreeItem* treeItem : children) {

        if (treeItem->name == name) {
            return treeItem;
        }
    }

    return nullptr;
}

void TreeItem::addChild(TreeItem *treeItem) {

    qsizetype i = 0;

    for (; i < children.length(); i++) {

        TreeItem* sibling = children[i];

        if (sibling->name.toUpper() > treeItem->name.toUpper()) {
            break;
        }
    }

    children.insert(i, treeItem);

    treeItem->parent = this;
}
