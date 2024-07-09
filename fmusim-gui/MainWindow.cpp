#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QStringListModel>
#include <QDesktopServices>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QMessageBox>
#include "ModelVariablesItemModel.h"

extern "C" {
#include "FMIZip.h"
#include "FMISimulation.h"
#include "FMIEuler.h"
#include "FMICVode.h"
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

    ui->showPlotAction->setEnabled(false);

    setCurrentPage(ui->startPage);

    variablesListModel = new ModelVariablesItemModel(this);
    variablesListModel->setStartValues(&startValues);

    connect(variablesListModel, &ModelVariablesItemModel::plotVariableSelected, this, &MainWindow::addPlotVariable);
    connect(variablesListModel, &ModelVariablesItemModel::plotVariableDeselected, this, &MainWindow::removePlotVariable);

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

    // block the signals during the update
    ui->showSettingsAction->blockSignals(true);
    ui->showFilesAction->blockSignals(true);
    ui->showDocumentationAction->blockSignals(true);
    ui->showLogAction->blockSignals(true);
    ui->showPlotAction->blockSignals(true);

    // set the current page
    ui->stackedWidget->setCurrentWidget(page);

    // toggle the actions
    ui->showSettingsAction->setChecked(page == ui->settingsPage);
    ui->showFilesAction->setChecked(page == ui->filesPage);
    ui->showDocumentationAction->setChecked(page == ui->documenationPage);
    ui->showLogAction->setChecked(page == ui->logPage);
    ui->showPlotAction->setChecked(page == ui->plotPage);

    // un-block the signals during the update
    ui->showSettingsAction->blockSignals(false);
    ui->showFilesAction->blockSignals(false);
    ui->showDocumentationAction->blockSignals(false);
    ui->showLogAction->blockSignals(false);
    ui->showPlotAction->blockSignals(false);
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

    const int status = FMIExtractArchive(cstr, unzipdir);

    if (status) {
        QMessageBox::critical(this, "Failed to extract FMU", "Failed to extract FMU");
        return;
    }

    FMIPathAppend(modelDescriptionPath, unzipdir);
    FMIPathAppend(modelDescriptionPath, "modelDescription.xml");

    modelDescription = FMIReadModelDescription(modelDescriptionPath);

    if (!modelDescription) {
        QMessageBox::critical(this, "Failed to load Model Description", "Failed to load Model Description");
        return;
    }

    // Loading finished. Update the GUI.
    startValues.clear();

    plotVariables.clear();

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        if (variable->causality == FMIOutput) {
            plotVariables << variable;
        }
    }

    setWindowTitle("FMUSim GUI - " + filename);

    if (modelDescription->defaultExperiment && modelDescription->defaultExperiment->stopTime) {
        stopTimeLineEdit->setText(modelDescription->defaultExperiment->stopTime);
    }

    variablesListModel->setModelDescription(modelDescription);
    variablesListModel->setPlotVariables(&plotVariables);

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

    if (modelDescription->defaultExperiment) {

        const FMIDefaultExperiment *experiment = modelDescription->defaultExperiment;

        if (experiment->stepSize) {
            ui->outputIntervalLineEdit->setText(modelDescription->defaultExperiment->stepSize);
        }

        ui->relativeToleranceLineEdit->setText(experiment->tolerance ? experiment->tolerance : "1e-5");
    }

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
    ui->showSettingsAction->setEnabled(true);
    ui->showFilesAction->setEnabled(true);
    ui->showDocumentationAction->setEnabled(true);
    ui->showLogAction->setEnabled(true);
    ui->simulateAction->setEnabled(true);
    stopTimeLineEdit->setEnabled(true);
    interfaceTypeComboBox->setEnabled(true);

    setCurrentPage(ui->settingsPage);
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

    window->ui->logPlainTextEdit->appendPlainText(item);
}

void MainWindow::simulate() {

    ui->logPlainTextEdit->clear();

    double stopTime = stopTimeLineEdit->text().toDouble();
    double outputInterval;

    if (ui->outputIntervalRadioButton->isChecked()) {
        outputInterval = ui->outputIntervalLineEdit->text().toDouble();
    } else {
        outputInterval = stopTime / ui->maxSamplesLineEdit->text().toDouble();
    }

    FMISimulationSettings settings;

    settings.interfaceType            = interfaceTypeComboBox->currentText() == "Co-Simulation" ? FMICoSimulation : FMIModelExchange;
    //settings.visible                  = false;
    //settings.loggingOn                = ui->debugLoggingCheckBox->isChecked();
    settings.tolerance                = ui->relativeToleranceLineEdit->text().toDouble();
    settings.nStartValues             = 0;
    settings.startVariables           = NULL;
    settings.startValues              = NULL;
    settings.tolerance                = ui->relativeToleranceLineEdit->text().toDouble();
    settings.startTime                = ui->startTimeLineEdit->text().toDouble();

    settings.outputInterval           = outputInterval;
    settings.stopTime                 = stopTime;
    settings.earlyReturnAllowed       = false;
    settings.eventModeUsed            = false;
    settings.recordIntermediateValues = false;
    settings.initialFMUStateFile      = NULL;
    settings.finalFMUStateFile        = NULL;

    settings.nStartValues = startValues.count();
    settings.startVariables = (const FMIModelVariable**)calloc(settings.nStartValues, sizeof(FMIModelVariable*));
    settings.startValues = (const char**)calloc(settings.nStartValues, sizeof(char*));

    if (ui->solverComboBox->currentText() == "Euler") {
        settings.solverCreate = FMIEulerCreate;
        settings.solverFree   = FMIEulerFree;
        settings.solverStep   = FMIEulerStep;
        settings.solverReset  = FMIEulerReset;
    } else {
        settings.solverCreate = FMICVodeCreate;
        settings.solverFree   = FMICVodeFree;
        settings.solverStep   = FMICVodeStep;
        settings.solverReset  = FMICVodeReset;
    }

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

    QList<FMIModelVariable*> recordedVariables;

    for (size_t i = 0; i < modelDescription->nModelVariables; i++) {

        FMIModelVariable* variable = &modelDescription->modelVariables[i];

        if (variable->variability == FMITunable || variable->variability == FMIContinuous || variable->variability == FMIDiscrete) {
            recordedVariables << variable;
        }

    }

    FMIStaticInput* input = nullptr;

    if (ui->inputCheckBox->isChecked()) {
        const QString inputPath = ui->inputLineEdit->text();
        const std::wstring  wstr  = ui->inputLineEdit->text().toStdWString();
        input = FMIReadInput(modelDescription, (char*)wstr.c_str());
        if (!input) {
            ui->logPlainTextEdit->appendPlainText("Failed to load " + inputPath + ".");
        }
    }

    recorder = FMICreateRecorder(S, recordedVariables.size(), (const FMIModelVariable**)recordedVariables.data(), "result.csv");

    const FMIStatus status = FMISimulate(S, modelDescription, NULL, recorder, input, &settings);

    updatePlot();

    ui->showPlotAction->setEnabled(false);

    if (status == FMIOK) {
        setCurrentPage(ui->plotPage);
    } else {
        setCurrentPage(ui->logPage);
    }
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

void MainWindow::updatePlot() {

    if (!recorder) {
        return;
    }

    QString data;
    data += "var data = [\n";

    size_t k = 0;

    for (size_t i = 0; i < recorder->nVariables; i++) {

        const FMIModelVariable* variable = recorder->variables[i];

        const FMIVariableType type = variable->type;

        if (!plotVariables.contains(variable)) {
            continue;
        }

        QString x;
        QString y;

        for (size_t j = 0; j < recorder->nRows; j++) {

            Row* row = recorder->rows[j];

            if (j > 0) {
                x += ", ";
                y += ", ";
            }

            size_t index;

            for (index = 0; index < recorder->variableInfos[type]->nVariables; index++) {
                if (recorder->variableInfos[type]->variables[index] == variable) {
                    break;
                }
            }

            x += QString::number(row->time);

            switch (type) {
            case FMIFloat32Type:
            case FMIDiscreteFloat32Type:
            {
                const float* values = (float*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            case FMIFloat64Type:
            case FMIDiscreteFloat64Type:
            {
                const double* values = (double*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            case FMIInt8Type:
            {
                const int8_t* values = (int8_t*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            case FMIUInt8Type:
            {
                const uint8_t* values = (uint8_t*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            case FMIInt16Type:
            {
                const int16_t* values = (int16_t*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            case FMIUInt16Type:
            {
                const uint16_t* values = (uint16_t*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            case FMIInt32Type:
            {
                const int32_t* values = (int32_t*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            case FMIUInt32Type:
            {
                const uint32_t* values = (uint32_t*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            case FMIInt64Type:
            {
                const int64_t* values = (int64_t*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            case FMIUInt64Type:
            {
                const uint64_t* values = (uint64_t*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            case FMIBooleanType:
            {
                const bool* values = (bool*)row->values[type];
                y += QString::number(values[index]);
            }
            break;
            default:
                y += "0";
                break;
            }
        }

        if (k > 0) {
            data += ", ";
        }
        
        data += "{x: [" + x + "], y: [" + y + "], type: 'scatter', name: '', line: {color: '#248BD2', width: 1.5";


        if (variable->variability == FMIContinuous) {
            data += "}";
        } else if (type == FMIBooleanType) {
            data += ", shape: 'hv'}, fill: 'tozeroy'";
        } else {
            data += ", shape: 'hv'}";
        }

        if (k > 0) {
            const QString plotIndex = QString::number(k + 1);
            data += ", xaxis: 'x" + plotIndex + "', yaxis: 'y" + plotIndex + "'";
        }

        data += "}\n";

        k++;
    }

    data += "];\n";

    QString axes;

    k = 0;

    for (size_t i = 0; i < recorder->nVariables; i++) {

        const FMIModelVariable* variable = recorder->variables[i];

        if (!plotVariables.contains(variable)) {
            continue;
        }

        const QString name = QString::fromUtf8(variable->name);
        const QString colors = "color: '#fff', zerolinecolor: '#666'";

        const double segment = 1.0 / plotVariables.size();
        const double margin = 0.02;

        const QString domain = "[" + QString::number(k * segment + (k == 0 ? 0.0 : margin)) + ", " + QString::number((k + 1) * segment - (k == recorder->nVariables ? 0.0 : margin)) + "]";

        if (k == 0) {
            axes += "xaxis: {" + colors + "},";
            axes += "yaxis: {title: '" + name + "', " + colors + ", domain: " + domain + "},";
        } else {
            axes += "xaxis" + QString::number(k + 1) +  ": {" + colors + ", matches: 'x'},";
            axes += "yaxis" + QString::number(k + 1) +  ": {title: '" + name + "', " + colors + ", domain: " + domain + "},";
        }

        k++;
    }

    QString javaScript =
        data +
        "    var layout = {"
        "    showlegend: false,"
        "    autosize: true,"
        "    font: {family: 'Segoe UI', size: 12},"
        "    plot_bgcolor: '#1e1e1e',"
        "    paper_bgcolor: '#1e1e1e',"
        "    grid: {rows: " + QString::number(plotVariables.size()) + ", columns: 1, pattern: 'independent'},"
                                                  "    template: 'plotly_dark'," + axes +
        "    color: '#0f0',"
        "    margin: { l: 60, r: 20, b: 20, t: 20, pad: 0 }"
        "};"
        "var config = {"
        "    'responsive': true"
        "};"
        "Plotly.newPlot('gd', data, layout, config);";

    qDebug() << javaScript;

    ui->plotWebEngineView->page()->runJavaScript(javaScript);
}

void MainWindow::addPlotVariable(const FMIModelVariable* variable) {
    plotVariables.append(variable);
    variablesListModel->setPlotVariables(&plotVariables);
    updatePlot();
}

void MainWindow::removePlotVariable(const FMIModelVariable* variable) {
    plotVariables.removeAll(variable);
    variablesListModel->setPlotVariables(&plotVariables);
    updatePlot();
}
