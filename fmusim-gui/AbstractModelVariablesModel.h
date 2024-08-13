#ifndef ABSTRACTMODELVARIABLESMODEL_H
#define ABSTRACTMODELVARIABLESMODEL_H

#include <QAbstractItemModel>
#include "FMIModelDescription.h"

class AbstractModelVariablesModel : public QAbstractItemModel {

    Q_OBJECT

public:

    enum ColumnIndex {
        NameColumn,
        TypeColumn,
        DimensionColumn,
        ValueReferenceColumn,
        InitialColumn,
        CausalityColumn,
        VariabilityColumn,
        StartColumn,
        NominalColumn,
        MinColumn,
        MaxColumn,
        UnitColumn,
        PlotColumn,
        DescriptionColumn,
        NumberOfColumns
    };

    explicit AbstractModelVariablesModel(QObject *parent = nullptr);

    void setModelDescription(const FMIModelDescription* modelDescription);

    void setStartValues(QMap<const FMIModelVariable*, QString> *startValues);

    void setPlotVariables(QList<const FMIModelVariable*> *plotVariables);

signals:
    void plotVariableSelected(const FMIModelVariable* variable);
    void plotVariableDeselected(const FMIModelVariable* variable);

protected:
    const FMIModelDescription* modelDescription = nullptr;
    QMap<const FMIModelVariable*, QString> *startValues = nullptr;
    QList<const FMIModelVariable*> *plotVariables = nullptr;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QVariant columnData(const FMIModelVariable* variable, int column, int role) const;

    virtual const FMIModelVariable* variableForIndex(const QModelIndex &index) const = 0;

};

#endif // ABSTRACTMODELVARIABLESMODEL_H
