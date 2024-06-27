#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QStringListModel>
#include <QDesktopServices>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QMimeData>>
#include "ModelVariablesItemModel.h"

extern "C" {
#include "FMIZip.h"
#include "FMISimulation.h"
// #include "FMICSVRecorder.h"
//#include "FMIDemoRecorder.h"
//#include "FMICSVRecorder.h"
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
    stopTimeLineEdit->setFixedWidth(80);
    stopTimeValidator = new QDoubleValidator(this);
    stopTimeValidator->setBottom(0);
    stopTimeLineEdit->setValidator(stopTimeValidator);
    ui->toolBar->addWidget(stopTimeLineEdit);

    QWidget* spacer = new QWidget();
    spacer->setFixedWidth(10);
    ui->toolBar->addWidget(spacer);

    interfaceTypeComboBox = new QComboBox(this);
    interfaceTypeComboBox->setEnabled(false);
    interfaceTypeComboBox->addItem("Co-Simulation");
    interfaceTypeComboBox->addItem("Model Exchange");
    interfaceTypeComboBox->setToolTip("Interface type");
    interfaceTypeComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->toolBar->addWidget(interfaceTypeComboBox);

    QWidget* spacer2 = new QWidget();
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar->addWidget(spacer2);
    ui->toolBar->addAction(ui->showSideBarAction);

    connect(ui->showSideBarAction, &QAction::toggled, this, [this](bool checked) { ui->dockWidget->setVisible(checked); });

    connect(ui->openUnzipDirectoryAction, &QAction::triggered, this, &MainWindow::openUnzipDirectory);
    connect(ui->simulateAction, &QAction::triggered, this, &MainWindow::simulate);

    // hide the dock's title bar
    ui->dockWidget->setTitleBarWidget(new QWidget());

    connect(ui->openButton, &QPushButton::clicked, this, &MainWindow::openFile);

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

    connect(ui->inputPushButton, &QPushButton::clicked, this, &MainWindow::selectInputFile);

    // disable widgets
    ui->dockWidget->setHidden(true);
    ui->showSettingsAction->setEnabled(false);
    ui->showFilesAction->setEnabled(false);
    ui->showDocumentationAction->setEnabled(false);
    ui->showLogAction->setEnabled(false);
    ui->showPlotAction->setEnabled(false);
    ui->simulateAction->setEnabled(false);
    ui->showSideBarAction->setEnabled(false);
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

    // Loading finished. Update the GUI.
    startValues.clear();

    setWindowTitle("FMUSim GUI - " + filename);

    if (modelDescription->defaultExperiment && modelDescription->defaultExperiment->stopTime) {
        stopTimeLineEdit->setText(modelDescription->defaultExperiment->stopTime);
    }

    variablesListModel->setModelDescription(modelDescription);

    ui->FMIVersionLabel->setText(modelDescription->fmiVersion);

    QStringList interfaces;

    if (modelDescription->modelExchange) {
        interfaces << "Model Exchange";
    }

    if (modelDescription->coSimulation) {
        interfaces << "Co-Simulation";
    }

    ui->interfacesLabel->setText(interfaces.join(", "));

    ui->variablesLabel->setText(QString::number(modelDescription->nModelVariables));
    ui->continuousStatesLabel->setText(QString::number(modelDescription->nContinuousStates));
    ui->eventIndicatorsLabel->setText(QString::number(modelDescription->nEventIndicators));

    ui->generationDateLabel->setText(modelDescription->generationDateAndTime);
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
    // ui->dockWidget->setHidden(false);
    ui->showSideBarAction->setEnabled(true);
    ui->showSideBarAction->toggle();
    ui->stackedWidget->setCurrentWidget(ui->settingsPage);
    ui->showSettingsAction->setEnabled(true);
    ui->showFilesAction->setEnabled(true);
    ui->showDocumentationAction->setEnabled(true);
    ui->showLogAction->setEnabled(true);
    ui->showPlotAction->setEnabled(true);
    ui->simulateAction->setEnabled(true);
    stopTimeLineEdit->setEnabled(true);
    interfaceTypeComboBox->setEnabled(true);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {

    for (const QUrl &url : event->mimeData()->urls()) {

        if (!url.isLocalFile()) {
            return;
        }
    }

    qDebug() << "accepting " << event;

    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {

    MainWindow* window = nullptr;

    for (const QUrl &url : event->mimeData()->urls()) {

        window = !window ? this : new MainWindow();

        window->loadFMU(url.toLocalFile());
    }
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

    settings.interfaceType            = interfaceTypeComboBox->currentText() == "Co-Simulation" ? FMICoSimulation : FMIModelExchange;
    //settings.visible                  = false;
    //settings.loggingOn                = ui->debugLoggingCheckBox->isChecked();
    settings.tolerance                = 0;
    settings.nStartValues             = 0;
    settings.startVariables           = NULL;
    settings.startValues              = NULL;
    settings.startTime                = ui->startTimeLineEdit->text().toDouble();
    settings.outputInterval           = 0.05;
    settings.stopTime                 = stopTimeLineEdit->text().toDouble();
    settings.earlyReturnAllowed       = false;
    settings.eventModeUsed            = false;
    settings.recordIntermediateValues = false;
    settings.initialFMUStateFile      = NULL;
    settings.finalFMUStateFile        = NULL;

    //settings.nStartValues = startValues.count();
    //settings.startVariables = (FMIModelVariable**)calloc(settings.nStartValues, sizeof(FMIModelVariable*));
    //settings.startValues = (char**)calloc(settings.nStartValues, sizeof(char*));

    size_t i = 0;

    for (auto [variable, value] : startValues.asKeyValueRange()) {
        settings.startVariables[i] = (FMIModelVariable*)variable;
        QByteArray buffer = value.toLocal8Bit();
        settings.startValues[i] = _strdup(buffer.data());
        i++;
    }

    char platformBinaryPath[FMI_PATH_MAX] = "";

    auto ba = unzipdir.toLocal8Bit();

    FMIPlatformBinaryPath(ba.data(), modelDescription->coSimulation->modelIdentifier, modelDescription->fmiMajorVersion, platformBinaryPath, FMI_PATH_MAX);

    FMIInstance *S = FMICreateInstance("instance1", logMessage, ui->logFMICallsCheckBox->isChecked() ? logFunctionCall : nullptr);

    if (!S) {
        printf("Failed to create FMU instance.\n");
        return;
    }

    S->userData = this;

    FMILoadPlatformBinary(S, platformBinaryPath);

    size_t nOutputVariables = 2;

    FMIModelVariable* outputVariables[2];
    outputVariables[0] = &modelDescription->modelVariables[1];
    outputVariables[1] = &modelDescription->modelVariables[3];

    // FMIModelVariable** outputVariables = variables;

    FMIStaticInput* input = nullptr;

    if (ui->inputCheckBox->isChecked()) {
        const QString inputPath = ui->inputLineEdit->text();
        const std::wstring  wstr  = ui->inputLineEdit->text().toStdWString();
        input = FMIReadInput(modelDescription, (char*)wstr.c_str());
        if (!input) {
            ui->listWidget->addItem("Failed to load " + inputPath + ".");
        }
    }

    FMIRecorder *recorder = FMICreateRecorder(0, NULL, "result.csv");

    const FMIStatus status = FMISimulate(S, modelDescription, NULL, recorder, input, &settings);

    // size_t nRows;
    // const double* values = FMIDemoRecorderValues(settings.recorder, &nRows);

    // QString x;
    // QString y;

    // for (size_t i = 0; i < nRows; i++) {

    //     if (i > 0) {
    //         x += ", ";
    //         y += ", ";
    //     }

    //     x += QString::number(values[2 * i]);
    //     y += QString::number(values[2 * i + 1]);
    // }

    //FMIRecorder* recorder = settings.recorder;

    // for (size_t i = 0; i < recorder->nRows; i++) {

    //     Row* row = recorder->rows[i];

    //     if (i > 0) {
    //         x += ", ";
    //         y += ", ";
    //     }

    //     x += QString::number(row->time);
    //     y += QString::number(row->float64Values[0]);
    // }



    // ui->plotWebEngineView->page()->runJavaScript("Plotly.newPlot('gd', { 'data': [{ 'y': [" + y + "] }], 'layout': { 'autosize': true }, 'config': { 'responsive': true, 'theme': 'ggplot2' } })");
    // ui->plotWebEngineView->page()->runJavaScript("Plotly.newPlot('gd', { 'data': [{ 'x': [" + x + "], 'y': [" + y + "] }] })");
    // ui->plotWebEngineView->page()->runJavaScript("Plotly.newPlot('gd', { 'data': [{ 'x': [" + x + "], 'y': [" + y + "] }], 'layout': { 'width': 600, 'height': 400} } })");

    // QString data;
    // data += "var data = [";

    // for (size_t i = 0; i < recorder->nVariables; i++) {

    //     const FMIModelVariable* variable = recorder->variables[i];

    //     //data += "  {x: [" + x + "], y: [" + y + "], type: 'scatter'},";
    //     //data += "  {x: [" + x + "], y: [" + y + "], type: 'scatter', xaxis: 'x2', yaxis: 'y2'}";

    //     QString x;
    //     QString y;

    //     for (size_t j = 0; j < recorder->nRows; j++) {

    //         Row* row = recorder->rows[j];

    //         if (j > 0) {
    //             x += ", ";
    //             y += ", ";
    //         }

    //         x += QString::number(row->time);
    //         y += QString::number(row->float64Values[i]);
    //     }

    //     if (i > 0) {
    //         data += ", ";
    //     }

    //     data += "{x: [" + x + "], y: [" + y + "], type: 'scatter', name: '" + variable->name + "'";

    //     if (i > 0) {
    //         const QString plotIndex = QString::number(i + 1);
    //         data += ", xaxis: 'x" + plotIndex + "', yaxis: 'y" + plotIndex + "'";
    //     }

    //     data += "}";
    // }

    // data += "];";

    // QString axes;

    // for (size_t i = 0; i < recorder->nVariables; i++) {

    //     const FMIModelVariable* variable = recorder->variables[i];

    //     const QString name = QString::fromUtf8(variable->name);
    //     const QString colors = "color: '#fff', zerolinecolor: '#666'";

    //     const double segment = 1.0 / recorder->nVariables;
    //     const double margin = 0.02;

    //     const QString domain = "[" + QString::number(i * segment + (i == 0 ? 0.0 : margin)) + ", " + QString::number((i + 1) * segment - (i == recorder->nVariables ? 0.0 : margin)) + "]";

    //     if (i == 0) {
    //         axes += "xaxis: {" + colors + ", matches: 'x" + QString::number(recorder->nVariables) + "'},";
    //         axes += "yaxis: {title: '" + name + "', " + colors + ", domain: " + domain + "},";
    //     } else {
    //         axes += "xaxis2: {" + colors + "},";
    //         axes += "yaxis2: {title: '" + name + "', " + colors + ", domain: " + domain + "},";
    //     }
    // }

    // qDebug() << axes;

    /*
    var trace1 = {
        x: [0, 1, 2, 3],
        y: [0, 4, 5, 6],
        type: 'scatter',
        name: 'height [m]'
    };

    var trace2 = {
        ygap: 0,
        x: [0, 1, 2, 3],
        y: [50, 60, 70, -10],
        xaxis: 'x2',
        yaxis: 'y2',
        type: 'scatter',
        name: 'velocity [m/s]'
    };

    var data = [trace1, trace2];
    */

    // QString javaScript =
    //     data +
    // "    var layout = {"
    // "    showlegend: false,"
    // "    autosize: true,"
    // "    font_color: '#0ff',"
    // "    plot_bgcolor: '#1e1e1e',"
    // "    paper_bgcolor: '#1e1e1e',"
    // "    grid: {rows: 2, columns: 1, pattern: 'independent'},"
    // "    template: 'plotly_dark'," + axes +
    // // "    xaxis: {color: '#fff', zerolinecolor: '#666', matches: 'x2', showticklabels: true},"
    // // "    yaxis: {title: 'height [m]', color: '#fff', zerolinecolor: '#666', domain: [0.52, 1.0]},"
    // // "    xaxis2: {color: '#fff', zerolinecolor: '#666', range: [-0.05, 3.05]},"
    // // "    yaxis2: {title: 'velocity [m/s]', color: '#fff', zerolinecolor: '#666', domain: [0.0, 0.48]},"
    // "    color: '#0f0',"
    // "    margin: { l: 60, r: 20, b: 30, t: 20, pad: 0 }"
    // "};"
    // "var config = {"
    // "    'responsive': true"
    // "};"
    // "Plotly.newPlot('gd', data, layout, config);";

    // ui->plotWebEngineView->page()->runJavaScript(javaScript);
}

void MainWindow::openFile() {

    const QString filename = QFileDialog::getOpenFileName(this, "Open File", QDir::homePath(), "FMUs (*.fmu);;All Files (*.*)");

    if (!filename.isEmpty()) {
        loadFMU(filename);
    }
}

void MainWindow::selectInputFile() {

    const QString filename = QFileDialog::getOpenFileName(this, "Open File", QDir::homePath(), "CSVs (*.csv);;All Files (*.*)");

    if (!filename.isEmpty()) {
        ui->inputLineEdit->setText(filename);
    }
}

void MainWindow::openUnzipDirectory() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(unzipdir));
}

void MainWindow::openFileInDefaultApplication(const QModelIndex &index) {
    const QString path = filesModel.filePath(index);
    QDesktopServices::openUrl(QUrl(path));
}
