#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QComboBox>
#include <QFileSystemModel>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

extern "C" {
#include "FMIModelDescription.h"
#include "FMIBuildDescription.h"
}

class ModelVariablesTableModel;
class ModelVariablesTreeModel;
class VariablesFilterModel;
class SimulationThread;
class BuildPlatformBinaryThread;
class QProgressDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadFMU(const QString &filename);
    void setColorScheme(Qt::ColorScheme colorScheme);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    Ui::MainWindow *ui;
    Qt::ColorScheme colorScheme = Qt::ColorScheme::Dark;
    QLineEdit *stopTimeLineEdit;
    QDoubleValidator *stopTimeValidator;
    QFileSystemModel filesModel;
    QComboBox* interfaceTypeComboBox;
    FMIModelDescription* modelDescription = nullptr;
    FMIBuildDescription* buildDescription = nullptr;
    QString unzipdir;
    QMap<const FMIModelVariable*, QString> startValues;
    QList<const FMIModelVariable*> plotVariables;
    ModelVariablesTableModel* variablesListModel = nullptr;
    VariablesFilterModel* variablesFilterModel = nullptr;
    ModelVariablesTreeModel* modelVariablesTreeModel = nullptr;
    SimulationThread* simulationThread = nullptr;
    BuildPlatformBinaryThread* buildPlatformBinaryThread = nullptr;
    QProgressDialog* progressDialog;
    QProgressDialog* buildPlatformBinaryProgressDialog;

    static MainWindow* currentMainWindow;
    static void logMessage(const char* message, va_list args);

    void setCurrentPage(QWidget *page);
    void updatePlot();
    void unloadFMU();

private slots:
    void openFile();
    void selectInputFile();
    void openUnzipDirectory();
    void openFileInDefaultApplication(const QModelIndex &index);
    void simulate();
    void addPlotVariable(const FMIModelVariable* variable);
    void removePlotVariable(const FMIModelVariable* variable);
    void setOptionalColumnsVisible(bool visible);
    void simulationFinished();
    void buildPlatformBinary();
    void showModelVariablesListView(bool show);
};
#endif // MAINWINDOW_H
