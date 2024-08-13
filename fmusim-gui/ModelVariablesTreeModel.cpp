#include <QIcon>
#include "ModelVariablesTreeModel.h"


ModelVariablesTreeModel::ModelVariablesTreeModel(QObject *parent)
    : AbstractModelVariablesModel(parent)
{
    rootItem = new TreeItem();
    rootItem->name = "root";
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

QVariant ModelVariablesTreeModel::data(const QModelIndex &index, int role) const {

    if (!index.isValid()) {
        return QVariant();
    }

    const TreeItem* treeItem = static_cast<TreeItem*>(index.internalPointer());

    if (index.column() == NameColumn && role == Qt::DisplayRole) {
        return treeItem->name;
    }

    if (treeItem->modelVariable) {
        return columnData(treeItem->modelVariable, index.column(), role);
    }

    if (index.column() == NameColumn && role == Qt::DecorationRole) {
        return QIcon(":/variables/dark/subsystem.svg");
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

const FMIModelVariable *ModelVariablesTreeModel::variableForIndex(const QModelIndex &index) const {

    const TreeItem* treeItem = static_cast<TreeItem*>(index.internalPointer());

    return treeItem->modelVariable;
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
