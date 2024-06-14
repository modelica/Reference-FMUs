#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QStringListModel>
#include <QDesktopServices>
#include <QFileDialog>
#include "ModelVariablesItemModel.h"

extern "C" {
#include "FMIZip.h"
#include "FMIUtil.h"
#include "FMI3.h"
#include "fmusim_fmi3_cs.h"
// #include "FMICSVRecorder.h"
#include "FMIDemoRecorder.h"
}

#define FMI_PATH_MAX 4096

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QIcon::setThemeName("light");

    ui->setupUi(this);

    // add the simulation controls to the toolbar
    stopTimeLineEdit = new QLineEdit(this);
    // stopTimeLineEdit->setEnabled(false);
    stopTimeLineEdit->setText("10.0");
    stopTimeLineEdit->setToolTip("Stop time");
    stopTimeLineEdit->setFixedWidth(50);
    stopTimeValidator = new QDoubleValidator(this);
    stopTimeValidator->setBottom(0);
    stopTimeLineEdit->setValidator(stopTimeValidator);
    ui->toolBar->addWidget(stopTimeLineEdit);

    QWidget* spacer = new QWidget(this);
    spacer->setFixedWidth(10);
    ui->toolBar->addWidget(spacer);

    interfaceTypeComboBox = new QComboBox(this);
    interfaceTypeComboBox->setEnabled(false);
    interfaceTypeComboBox->addItem("Co-Simulation");
    interfaceTypeComboBox->addItem("Model Exchange");
    interfaceTypeComboBox->setToolTip("Interface type");
    interfaceTypeComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->toolBar->addWidget(interfaceTypeComboBox);

    connect(ui->openUnzipDirectoryAction, &QAction::triggered, this, &MainWindow::openUnzipDirectory);
    connect(ui->simulateAction, &QAction::triggered, this, &MainWindow::simulate);

    // hide the dock's title bar
    ui->dockWidget->setTitleBarWidget(new QWidget());

    //ui->filesTreeView->resizeColumnToContents(0);
    //ui->filesTreeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    connect(ui->filesTreeView, &QAbstractItemView::doubleClicked, this, &MainWindow::openFileInDefaultApplication);

    connect(ui->openFileAction,          &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->showSettingsAction,      &QAction::triggered, this, [this]() { setCurrentPage(ui->settingsPage);     });
    connect(ui->showFilesAction,         &QAction::triggered, this, [this]() { setCurrentPage(ui->filesPage);        });
    connect(ui->showDocumentationAction, &QAction::triggered, this, [this]() { setCurrentPage(ui->documenationPage); });
    connect(ui->showLogAction,           &QAction::triggered, this, [this]() { setCurrentPage(ui->logPage);          });
    connect(ui->showPlotAction,          &QAction::triggered, this, [this]() { setCurrentPage(ui->plotPage);         });

    variablesListModel = new ModelVariablesItemModel(this);
    variablesListModel->setStartValues(&startValues);

    ui->treeView->setModel(variablesListModel);

    // ui->treeView->setColumnWidth(0, ModelVariablesItemModel::NAME_COLUMN_DEFAULT_WIDTH);
    // ui->treeView->setColumnWidth(1, ModelVariablesItemModel::START_COLUMN_DEFAULT_WIDTH);
    const static int COLUMN_WIDTHS[] = {200, 50, 70, 100, 70, 70, 70, 70, 70, 70, 70, 40, 40};

    for (size_t i = 0; i < ModelVariablesItemModel::NUMBER_OF_COLUMNS - 1; i++) {
        ui->treeView->setColumnWidth(i, COLUMN_WIDTHS[i]);
    }

    // hide columns
    ui->treeView->hideColumn(ModelVariablesItemModel::TYPE_COLUMN_INDEX);
    ui->treeView->hideColumn(ModelVariablesItemModel::DIMENSION_COLUMN_INDEX);
    ui->treeView->hideColumn(ModelVariablesItemModel::VALUE_REFERENCE_COLUMN_INDEX);
    ui->treeView->hideColumn(ModelVariablesItemModel::INITIAL_COLUMN_INDEX);
    ui->treeView->hideColumn(ModelVariablesItemModel::CAUSALITY_COLUMN_INDEX);
    ui->treeView->hideColumn(ModelVariablesItemModel::VARIABITLITY_COLUMN_INDEX);
    ui->treeView->hideColumn(ModelVariablesItemModel::NOMINAL_COLUMN_INDEX);
    ui->treeView->hideColumn(ModelVariablesItemModel::MIN_COLUMN_INDEX);
    ui->treeView->hideColumn(ModelVariablesItemModel::MAX_COLUMN_INDEX);

    // disable widgets
    ui->showSettingsAction->setEnabled(false);
    ui->showFilesAction->setEnabled(false);
    ui->showDocumentationAction->setEnabled(false);
    ui->showLogAction->setEnabled(false);
    ui->showPlotAction->setEnabled(false);
    ui->simulateAction->setEnabled(false);
    stopTimeLineEdit->setEnabled(false);
    interfaceTypeComboBox->setEnabled(false);
}

void MainWindow::setCurrentPage(QWidget *page) {
    ui->stackedWidget->setCurrentWidget(page);
}

MainWindow::~MainWindow()
{
    delete ui;

    if (modelDescription) {
        FMIFreeModelDescription(modelDescription);
    }

    if (!unzipdir.isEmpty()) {
        QByteArray bytes = unzipdir.toLocal8Bit();
        const char *cstr = bytes.data();
        FMIRemoveDirectory(cstr);
    }
}

void MainWindow::loadFMU(const QString &filename) {

    const char* unzipdir = FMICreateTemporaryDirectory();

    this->unzipdir = unzipdir;

    char modelDescriptionPath[FMI_PATH_MAX] = "";

    QByteArray bytes = filename.toLocal8Bit();
    const char *cstr = bytes.data();

    int status = FMIExtractArchive(cstr, unzipdir);

    FMIPathAppend(modelDescriptionPath, unzipdir);
    FMIPathAppend(modelDescriptionPath, "modelDescription.xml");

    modelDescription = FMIReadModelDescription(modelDescriptionPath);

    switch (modelDescription->fmiVersion) {
    case FMIVersion1:
        ui->FMIVersionLabel->setText("1.0");
        break;
    case FMIVersion2:
        ui->FMIVersionLabel->setText("2.0");
        break;
    case FMIVersion3:
        ui->FMIVersionLabel->setText("3.0");
        break;
    }

    // Loading finished. Update the GUI.
    startValues.clear();

    setWindowTitle("FMUSim GUI - " + filename);

    if (modelDescription->defaultExperiment && modelDescription->defaultExperiment->stopTime) {
        stopTimeLineEdit->setText(modelDescription->defaultExperiment->stopTime);
    }

    variablesListModel->setModelDescription(modelDescription);

    ui->variablesLabel->setText(QString::number(modelDescription->nModelVariables));
    ui->continuousStatesLabel->setText(QString::number(modelDescription->nContinuousStates));
    ui->eventIndicatorsLabel->setText(QString::number(modelDescription->nEventIndicators));

    ui->generationDateLabel->setText(modelDescription->generationDate);
    ui->generationToolLabel->setText(modelDescription->generationTool);
    ui->descriptionLabel->setText(modelDescription->description);

    filesModel.setRootPath(this->unzipdir);

    auto rootIndex = filesModel.index(this->unzipdir);

    ui->filesTreeView->setModel(&filesModel);
    ui->filesTreeView->setRootIndex(rootIndex);
    ui->filesTreeView->setColumnWidth(0, 250);

    const QString doc = QDir::cleanPath(this->unzipdir + QDir::separator() + "documentation" + QDir::separator() + "index.html");
    ui->documentationWebEngineView->load(QUrl::fromLocalFile(doc));

    ui->plotWebEngineView->load(QUrl("qrc:/plot.html"));

    // enable widgets
    ui->showSettingsAction->setEnabled(true);
    ui->showFilesAction->setEnabled(true);
    ui->showDocumentationAction->setEnabled(true);
    ui->showLogAction->setEnabled(true);
    ui->showPlotAction->setEnabled(true);
    ui->simulateAction->setEnabled(true);
    stopTimeLineEdit->setEnabled(true);
    interfaceTypeComboBox->setEnabled(true);
}

static void logMessage(FMIInstance* instance, FMIStatus status, const char* category, const char* message) {

    switch (status) {
    case FMIOK:
        printf("[OK] ");
        break;
    case FMIWarning:
        printf("[Warning] ");
        break;
    case FMIDiscard:
        printf("[Discard] ");
        break;
    case FMIError:
        printf("[Error] ");
        break;
    case FMIFatal:
        printf("[Fatal] ");
        break;
    case FMIPending:
        printf("[Pending] ");
        break;
    }

    puts(message);
}

void MainWindow::logFunctionCall(FMIInstance* instance, FMIStatus status, const char* message) {

    MainWindow *window = (MainWindow *)instance->userData;

    QString item(message);


    switch (status) {
    case FMIOK:
        item += " -> OK";
        break;
    case FMIWarning:
        item += " -> Warning";
        break;
    case FMIDiscard:
        item += " -> Discard";
        break;
    case FMIError:
        item += " -> Error";
        break;
    case FMIFatal:
        item += " -> Fatal";
        break;
    case FMIPending:
        item += " -> Pending";
        break;
    default:
        item += " -> Unknown status";
        break;
    }

    window->ui->listWidget->addItem(item);
}

void MainWindow::simulate() {

    FMISimulationSettings settings;

    settings.tolerance                = 0;
    settings.nStartValues             = 0;
    settings.startVariables           = NULL;
    settings.startValues              = NULL;
    settings.startTime                = 0.0;
    settings.outputInterval           = 0.01;
    settings.stopTime                 = stopTimeLineEdit->text().toDouble();
    settings.earlyReturnAllowed       = false;
    settings.eventModeUsed            = false;
    settings.recordIntermediateValues = false;
    settings.initialFMUStateFile      = NULL;
    settings.finalFMUStateFile        = NULL;

    settings.nStartValues = startValues.count();
    settings.startVariables = (FMIModelVariable**)calloc(settings.nStartValues, sizeof(FMIModelVariable*));
    settings.startValues = (char**)calloc(settings.nStartValues, sizeof(char*));

    size_t i = 0;

    for (auto [variable, value] : startValues.asKeyValueRange()) {
        settings.startVariables[i] = (FMIModelVariable*)variable;
        QByteArray buffer = value.toLocal8Bit();
        settings.startValues[i] = _strdup(buffer.data());
        i++;
    }

    char platformBinaryPath[FMI_PATH_MAX] = "";

    auto ba = unzipdir.toLocal8Bit();

    FMIPlatformBinaryPath(ba.data(), modelDescription->coSimulation->modelIdentifier, modelDescription->fmiVersion, platformBinaryPath, FMI_PATH_MAX);

    FMIInstance *S = FMICreateInstance("instance1", logMessage, logFunctionCall);

    if (!S) {
        printf("Failed to create FMU instance.\n");
        return;
    }

    S->userData = this;

    FMILoadPlatformBinary(S, platformBinaryPath);

    size_t nOutputVariables = 0;


    FMIModelVariable* variables[2];
    variables[0] = &modelDescription->modelVariables[1];
    variables[1] = &modelDescription->modelVariables[3];

    // FMIModelVariable** outputVariables = variables;

    //FMIRecorder *recorder = FMICreateRecorder(0, NULL, "BouncingBall_out.csv");
    settings.recorder = FMIDemoRecorderCreate(S);
    settings.sample = FMIDemoRecorderSample;

    const FMIStatus status = simulateFMI3CS(S, modelDescription, NULL, NULL, &settings);

    size_t nRows;
    const double* values = FMIDemoRecorderValues(settings.recorder, &nRows);

    QString x;
    QString y;

    for (size_t i = 0; i < nRows; i++) {

        if (i > 0) {
            x += ", ";
            y += ", ";
        }

        x += QString::number(values[2 * i]);
        y += QString::number(values[2 * i + 1]);
    }

    ui->plotWebEngineView->page()->runJavaScript("Plotly.newPlot('gd', { 'data': [{ 'y': [" + y + "] }], 'layout': { 'autosize': true }, 'config': { 'responsive': true, 'theme': 'ggplot2' } })");
    // ui->plotWebEngineView->page()->runJavaScript("Plotly.newPlot('gd', { 'data': [{ 'x': [" + x + "], 'y': [" + y + "] }] })");
    // ui->plotWebEngineView->page()->runJavaScript("Plotly.newPlot('gd', { 'data': [{ 'x': [" + x + "], 'y': [" + y + "] }], 'layout': { 'width': 600, 'height': 400} } })");
}

void MainWindow::openFile() {

    const QString filename = QFileDialog::getOpenFileName(this, "Open File", QDir::homePath(), "FMUs (*.fmu);;All Files (*.*)");

    if (!filename.isEmpty()) {
        loadFMU(filename);
    }
}


void MainWindow::openUnzipDirectory() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(unzipdir));
}

void MainWindow::openFileInDefaultApplication(const QModelIndex &index) {
    const QString path = filesModel.filePath(index);
    QDesktopServices::openUrl(QUrl(path));
}
