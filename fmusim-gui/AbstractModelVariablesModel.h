#ifndef ABSTRACTMODELVARIABLESMODEL_H
#define ABSTRACTMODELVARIABLESMODEL_H

#include <QAbstractItemModel>
#include "FMIModelDescription.h"

class AbstractModelVariablesModel : public QAbstractItemModel {

    Q_OBJECT

public:
    const static int NUMBER_OF_COLUMNS = 14;

    const static int NAME_COLUMN_INDEX = 0;
    const static int TYPE_COLUMN_INDEX = 1;
    const static int DIMENSION_COLUMN_INDEX = 2;
    const static int VALUE_REFERENCE_COLUMN_INDEX = 3;
    const static int INITIAL_COLUMN_INDEX = 4;
    const static int CAUSALITY_COLUMN_INDEX = 5;
    const static int VARIABITLITY_COLUMN_INDEX = 6;
    const static int START_COLUMN_INDEX = 7;
    const static int NOMINAL_COLUMN_INDEX = 8;
    const static int MIN_COLUMN_INDEX = 9;
    const static int MAX_COLUMN_INDEX = 10;
    const static int UNIT_COLUMN_INDEX = 11;
    const static int PLOT_COLUMN_INDEX = 12;
    const static int DESCRIPTION_COLUMN_INDEX = 13;

    const static int NAME_COLUMN_DEFAULT_WIDTH = 200;
    const static int START_COLUMN_DEFAULT_WIDTH = 70;

    explicit AbstractModelVariablesModel(QObject *parent = nullptr);

protected:
    const FMIModelDescription* modelDescription = nullptr;
    QMap<const FMIModelVariable*, QString> *startValues = nullptr;
    QList<const FMIModelVariable*> *plotVariables = nullptr;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QVariant columnData(const FMIModelVariable* variable, int column, int role) const;

};

#endif // ABSTRACTMODELVARIABLESMODEL_H
