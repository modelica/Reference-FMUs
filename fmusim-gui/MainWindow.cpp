#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QStringListModel>
#include <QDesktopServices>
#include <QFileDialog>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QMessageBox>
#include <QStyleHints>
#include <QDirIterator>
#include <QProgressDialog>
#include <QSettings>
#include "ModelVariablesTableModel.h"
#include "ModelVariablesTreeModel.h"
#include "VariablesFilterModel.h"
#include "SimulationThread.h"
#include "BuildPlatformBinaryThread.h"
#include "BuildPlatformBinaryDialog.h"
#include "PlotUtil.h"

extern "C" {
#include "FMIZip.h"
#include "FMISimulation.h"
#include "FMIEuler.h"
#include "FMICVode.h"
}

#include <cstring>

#define FMI_PATH_MAX 4096


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{    
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    simulationThread = new SimulationThread(this);

    connect(simulationThread, &SimulationThread::plotChanged, this, &MainWindow::runPlotScript);
    connect(simulationThread, &SimulationThread::finished, this, &MainWindow::simulationFinished);
    connect(this, &MainWindow::stopSimulationRequested, simulationThread, &SimulationThread::stop);
    connect(this, &MainWindow::plotVariablesChanged, simulationThread, &SimulationThread::setPlotVariables);

    buildPlatformBinaryThread = new BuildPlatformBinaryThread(this);

    buildPlatformBinaryProgressDialog = new QProgressDialog(this);

    Qt::WindowFlags flags = buildPlatformBinaryProgressDialog->windowFlags();
    Qt::WindowFlags closeFlag = Qt::WindowCloseButtonHint;
    flags = flags & (~closeFlag);

    buildPlatformBinaryProgressDialog->setWindowTitle("FMUSim");
    buildPlatformBinaryProgressDialog->setLabelText("Building Platform Binary...");
    buildPlatformBinaryProgressDialog->setRange(0, 0);
    buildPlatformBinaryProgressDialog->setWindowModality(Qt::WindowModal);
    buildPlatformBinaryProgressDialog->setCancelButton(nullptr);

    buildPlatformBinaryProgressDialog->setWindowFlags(flags);
    buildPlatformBinaryProgressDialog->reset();

    connect(buildPlatformBinaryThread, &BuildPlatformBinaryThread::newMessage, ui->logPlainTextEdit, &QPlainTextEdit::appendPlainText);
    connect(buildPlatformBinaryThread, &BuildPlatformBinaryThread::finished, buildPlatformBinaryProgressDialog, &QProgressDialog::reset);

    setColorScheme(QGuiApplication::styleHints()->colorScheme());

    // context menu
    contextMenu = new QMenu(this);

    QAction* expandAllAction = contextMenu->addAction("Expand All");
    connect(expandAllAction, &QAction::triggered, ui->modelVariablesTreeView, &QTreeView::expandAll);

    QAction* collapseAllAction = contextMenu->addAction("Collapse All");
    connect(collapseAllAction, &QAction::triggered, ui->modelVariablesTreeView, &QTreeView::collapseAll);

    // recent files
    QVBoxLayout* vbox = new QVBoxLayout();

    ui->recentFilesGroupBox->setLayout(vbox);

    QSettings settings;

    const QStringList recentFiles = settings.value("recentFiles").toStringList();

    for (const QString& filename : recentFiles.mid(0, 5)) {
        QFileInfo fileInfo(filename);
        QLabel* link = new QLabel("<a href='" + QDir::toNativeSeparators(filename) + "' style='text-decoration: none; color: #248BD2'>" + fileInfo.fileName() +  "</a>");
        link->setToolTip(QDir::toNativeSeparators(filename));
        connect(link, &QLabel::linkActivated, this, &MainWindow::loadFMU);
        vbox->addWidget(link);
    }

    ui->recentFilesGroupBox->setVisible(!recentFiles.isEmpty());

    // load the web engine, so the window doesn't jump later
    ui->plotWebEngineView->setHtml("");

    // add the simulation controls to the toolbar
    stopTimeLineEdit = new QLineEdit(this);
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

    connect(ui->showSideBarAction, &QAction::toggled, this, [this](bool checked) {
        ui->dockWidget->setVisible(checked);
    });

    connect(ui->openUnzipDirectoryAction, &QAction::triggered, this, &MainWindow::openUnzipDirectory);
    connect(ui->simulateAction, &QAction::triggered, this, &MainWindow::simulate);

    // hide the dock's title bar
    ui->dockWidget->setTitleBarWidget(new QWidget());

    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->logPlainTextEdit->setFont(fixedFont);

    connect(ui->openButton, &QPushButton::clicked, this, &MainWindow::openFile);

    connect(ui->filesTreeView, &QAbstractItemView::doubleClicked, this, &MainWindow::openFileInDefaultApplication);

    connect(ui->openFileAction,          &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->newWindowAction,         &QAction::triggered, this, []()     { (new MainWindow())->show();           });
    connect(ui->showInfoAction,          &QAction::triggered, this, [this]() { setCurrentPage(ui->infoPage);         });
    connect(ui->showSettingsAction,      &QAction::triggered, this, [this]() { setCurrentPage(ui->settingsPage);     });
    connect(ui->showFilesAction,         &QAction::triggered, this, [this]() { setCurrentPage(ui->filesPage);        });
    connect(ui->showDocumentationAction, &QAction::triggered, this, [this]() { setCurrentPage(ui->documenationPage); });
    connect(ui->showLogAction,           &QAction::triggered, this, [this]() { setCurrentPage(ui->logPage);          });
    connect(ui->showPlotAction,          &QAction::triggered, this, [this]() { setCurrentPage(ui->plotPage);         });

    connect(ui->buildPlatformBinaryAction, &QAction::triggered, this, &MainWindow::buildPlatformBinary);

    setCurrentPage(ui->startPage);

    // table model
    variablesListModel = new ModelVariablesTableModel(this);
    variablesListModel->setStartValues(&startValues);

    connect(variablesListModel, &ModelVariablesTableModel::plotVariableSelected, this, &MainWindow::addPlotVariable);
    connect(variablesListModel, &ModelVariablesTableModel::plotVariableDeselected, this, &MainWindow::removePlotVariable);

    variablesFilterModel = new VariablesFilterModel();

    variablesFilterModel->setSourceModel(variablesListModel);

    ui->modelVariablesListView->setModel(variablesFilterModel);
    ui->modelVariablesListView->sortByColumn(0, Qt::SortOrder::AscendingOrder);

    // tree model
    modelVariablesTreeModel = new ModelVariablesTreeModel(this);
    modelVariablesTreeModel->setStartValues(&startValues);

    connect(modelVariablesTreeModel, &ModelVariablesTreeModel::plotVariableSelected, this, &MainWindow::addPlotVariable);
    connect(modelVariablesTreeModel, &ModelVariablesTreeModel::plotVariableDeselected, this, &MainWindow::removePlotVariable);

    ui->modelVariablesTreeView->setModel(modelVariablesTreeModel);

    connect(ui->modelVariablesTreeView, &QTreeView::customContextMenuRequested, this, [this](const QPoint &pos) {
        contextMenu->exec(ui->modelVariablesTreeView->mapToGlobal(pos));
    });

    // variable tool buttons
    connect(ui->filterLineEdit, &QLineEdit::textChanged, variablesFilterModel, &VariablesFilterModel::setFilterFixedString);
    connect(ui->filterParameterVariablesToolButton, &QToolButton::clicked, variablesFilterModel, &VariablesFilterModel::setFilterParamterVariables);
    connect(ui->filterInputVariablesToolButton, &QToolButton::clicked, variablesFilterModel, &VariablesFilterModel::setFilterInputVariables);
    connect(ui->filterOutputVariablesToolButton, &QToolButton::clicked, variablesFilterModel, &VariablesFilterModel::setFilterOutputVariables);
    connect(ui->filterLocalVariablesToolButton, &QToolButton::clicked, variablesFilterModel, &VariablesFilterModel::setFilterLocalVariables);
    connect(ui->clearPlotsToolButton, &QToolButton::clicked, this, [this]() {
        plotVariables.clear();
        variablesListModel->setPlotVariables(&plotVariables);
        modelVariablesTreeModel->setPlotVariables(&plotVariables);
        updatePlot();
    });
    connect(ui->showOptionalColumnsToolButton, &QToolButton::clicked, this, &MainWindow::setOptionalColumnsVisible);
    connect(ui->showListViewToolButton, &QToolButton::clicked, this, &MainWindow::showModelVariablesListView);

    connect(ui->inputPushButton, &QPushButton::clicked, this, &MainWindow::selectInputFile);

    unloadFMU();
}

void MainWindow::setCurrentPage(QWidget *page) {

    // block the signals during the update
    ui->showInfoAction->blockSignals(true);
    ui->showSettingsAction->blockSignals(true);
    ui->showFilesAction->blockSignals(true);
    ui->showDocumentationAction->blockSignals(true);
    ui->showLogAction->blockSignals(true);
    ui->showPlotAction->blockSignals(true);

    // set the current page
    ui->stackedWidget->setCurrentWidget(page);

    // toggle the actions
    ui->showInfoAction->setChecked(page == ui->infoPage);
    ui->showSettingsAction->setChecked(page == ui->settingsPage);
    ui->showFilesAction->setChecked(page == ui->filesPage);
    ui->showDocumentationAction->setChecked(page == ui->documenationPage);
    ui->showLogAction->setChecked(page == ui->logPage);
    ui->showPlotAction->setChecked(page == ui->plotPage);

    // un-block the signals during the update
    ui->showInfoAction->blockSignals(false);
    ui->showSettingsAction->blockSignals(false);
    ui->showFilesAction->blockSignals(false);
    ui->showDocumentationAction->blockSignals(false);
    ui->showLogAction->blockSignals(false);
    ui->showPlotAction->blockSignals(false);
}

MainWindow::~MainWindow() {
    delete ui;
    ui = nullptr;
    unloadFMU();
}

void MainWindow::logMessage(const char* message, va_list args) {

    const QString formatted = QString::vasprintf(message, args);

    if (currentMainWindow) {
        currentMainWindow->ui->logPlainTextEdit->appendPlainText(formatted);
    } else {
        qDebug() << formatted;
    }
}

void MainWindow::loadFMU(const QString &filename) {

    currentMainWindow = this;

    logErrorMessage = MainWindow::logMessage;

    unloadFMU();

    const char* unzipdir = FMICreateTemporaryDirectory();

    if (!unzipdir) {
        ui->logPlainTextEdit->appendPlainText("Failed to create temporary directory.");
        setCurrentPage(ui->logPage);
        return;
    }

    this->unzipdir = unzipdir;

    QDir::setCurrent(unzipdir);

    char modelDescriptionPath[FMI_PATH_MAX] = "";

    QByteArray bytes = filename.toLocal8Bit();
    const char *cstr = bytes.data();

    const int status = FMIExtractArchive(cstr, unzipdir);

    if (status != 0) {
        ui->logPlainTextEdit->appendPlainText("Failed to extract FMU.");
        setCurrentPage(ui->logPage);
        return;
    }

    FMIPathAppend(modelDescriptionPath, unzipdir);
    FMIPathAppend(modelDescriptionPath, "modelDescription.xml");

    QDir binariesDir(QString(unzipdir) + QDir::separator() + "binaries");

    QList<QString> platforms;

    for(const QFileInfo &fileInfo: binariesDir.entryInfoList()) {
        if (fileInfo.isDir() && !fileInfo.fileName().startsWith(".")) {
            platforms << fileInfo.fileName();
        }
    }

    ui->platformsLabel->setText(platforms.join(", "));

    modelDescription = FMIReadModelDescription(modelDescriptionPath);

    if (!modelDescription) {
        ui->logPlainTextEdit->appendPlainText("Failed to load Model Description.");
        setCurrentPage(ui->logPage);
        return;
    }

    const QString buildDescriptionPath = QDir(unzipdir).filePath("sources/buildDescription.xml");

    if (QFileInfo::exists(buildDescriptionPath)) {
        QByteArray ba = buildDescriptionPath.toLocal8Bit();
        buildDescription = FMIReadBuildDescription(ba.data());
    }

    // update the GUI
    startValues.clear();

    plotVariables.clear();

    for (size_t i = 0; i < modelDescription->nModelVariables && plotVariables.size() < 5; i++) {

        const FMIModelVariable* variable = modelDescription->modelVariables[i];

        if (variable->causality == FMIOutput) {
            plotVariables << variable;
        }
    }

    if (plotVariables.isEmpty()) {

        for (size_t i = 0; i < modelDescription->nModelVariables && plotVariables.size() < 5; i++) {

            const FMIModelVariable* variable = modelDescription->modelVariables[i];

            if (variable->variability == FMIContinuous) {
                plotVariables << variable;
            }
        }
    }

    setWindowTitle("FMUSim " + QApplication::applicationVersion() + " - " + filename);

    const bool structured = modelDescription->variableNamingConvention == FMIStructured;

    ui->showListViewToolButton->setEnabled(structured);
    ui->showListViewToolButton->setChecked(!structured);
    showModelVariablesListView(!structured);

    if (modelDescription->defaultExperiment && modelDescription->defaultExperiment->stopTime) {
        stopTimeLineEdit->setText(modelDescription->defaultExperiment->stopTime);
    }

    variablesListModel->setModelDescription(modelDescription);
    variablesListModel->setPlotVariables(&plotVariables);

    modelVariablesTreeModel->setModelDescription(modelDescription);
    modelVariablesTreeModel->setPlotVariables(&plotVariables);

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

    if (QFileInfo::exists(doc)) {
        ui->showDocumentationAction->setEnabled(true);
        ui->documentationWebEngineView->load(QUrl::fromLocalFile(doc));
    } else {
        ui->showDocumentationAction->setEnabled(false);
    }

    ui->plotWebEngineView->load(QUrl("qrc:/plot.html"));

    // enable widgets
    ui->dockWidget->show();
    ui->showSideBarAction->setEnabled(true);
    ui->showSideBarAction->setChecked(true);

    bool hasSourceCode = false;

    if ((modelDescription->coSimulation && modelDescription->coSimulation->nSourceFiles > 0) ||
        (modelDescription->modelExchange && modelDescription->modelExchange->nSourceFiles > 0) ||
        buildDescription) {
        hasSourceCode = true;
    }

    ui->buildPlatformBinaryAction->setEnabled(hasSourceCode);

    ui->showInfoAction->setEnabled(true);
    ui->showSettingsAction->setEnabled(true);
    ui->showFilesAction->setEnabled(true);
    ui->showLogAction->setEnabled(true);
    ui->showPlotAction->setEnabled(false);
    ui->simulateAction->setEnabled(true);

    stopTimeLineEdit->setEnabled(true);

    interfaceTypeComboBox->setCurrentText(modelDescription->coSimulation ? "Co-Simulation" : "Model Exchange");
    interfaceTypeComboBox->setEnabled(modelDescription->modelExchange && modelDescription->coSimulation);

    const QString modelImage = QString(unzipdir) + QDir::separator() + "model.png";

    ui->imageLabel->setPixmap(QPixmap(modelImage));

    setCurrentPage(ui->infoPage);

    QSettings settings;

    QStringList recentFiles = settings.value("recentFiles").toStringList();

    recentFiles.removeAll(filename);

    recentFiles.prepend(filename);

    settings.setValue("recentFiles", recentFiles.mid(0, 5));
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

void MainWindow::changeEvent(QEvent *event) {

    if (event->type() == QEvent::Type::StyleChange) {
        QStyleHints *styleHints = QGuiApplication::styleHints();
        setColorScheme(styleHints->colorScheme());
    }

    QMainWindow::changeEvent(event);
}

MainWindow* MainWindow::currentMainWindow = nullptr;

void MainWindow::setColorScheme(Qt::ColorScheme colorScheme) {

    if (this->colorScheme == colorScheme) {
        return;
    }

    const QString theme = colorScheme == Qt::ColorScheme::Dark ? "dark" : "light";

    // toolbar
    ui->newWindowAction->setIcon(QIcon(":/buttons/" + theme + "/new-window.svg"));
    ui->buildPlatformBinaryAction->setIcon(QIcon(":/buttons/" + theme + "/hammer.svg"));
    ui->openFileAction->setIcon(QIcon(":/buttons/" + theme + "/folder-open.svg"));
    ui->showInfoAction->setIcon(QIcon(":/buttons/" + theme + "/info.svg"));
    ui->showSettingsAction->setIcon(QIcon(":/buttons/" + theme + "/gear.svg"));
    ui->showFilesAction->setIcon(QIcon(":/buttons/" + theme + "/file-earmark-zip.svg"));
    ui->showDocumentationAction->setIcon(QIcon(":/buttons/" + theme + "/book.svg"));
    ui->showLogAction->setIcon(QIcon(":/buttons/" + theme + "/list-task.svg"));
    ui->showPlotAction->setIcon(QIcon(":/buttons/" + theme + "/graph.svg"));
    ui->simulateAction->setIcon(QIcon(":/buttons/" + theme + "/play.svg"));
    ui->showSideBarAction->setIcon(QIcon(":/buttons/" + theme + "/side-bar.svg"));

    // tool buttons
    ui->filterParameterVariablesToolButton->setIcon(QIcon(":/tools/" + theme + "/parameter.svg"));
    ui->filterInputVariablesToolButton->setIcon(QIcon(":/tools/" + theme + "/input-variable.svg"));
    ui->filterOutputVariablesToolButton->setIcon(QIcon(":/tools/" + theme + "/output-variable.svg"));
    ui->filterLocalVariablesToolButton->setIcon(QIcon(":/tools/" + theme + "/local-variable.svg"));
    ui->clearPlotsToolButton->setIcon(QIcon(":/tools/" + theme + "/broom.svg"));
    ui->showOptionalColumnsToolButton->setIcon(QIcon(":/tools/" + theme + "/columns.svg"));
    ui->showListViewToolButton->setIcon(QIcon(":/tools/" + theme + "/list.svg"));

    this->colorScheme = colorScheme;

    updatePlot();
}

void MainWindow::simulate() {

    if (simulationThread->isRunning()) {
        emit stopSimulationRequested();
        return;
    }

    ui->logPlainTextEdit->clear();

    double stopTime = stopTimeLineEdit->text().toDouble();
    double outputInterval;

    if (ui->outputIntervalRadioButton->isChecked()) {
        outputInterval = ui->outputIntervalLineEdit->text().toDouble();
    } else {
        outputInterval = stopTime / ui->maxSamplesLineEdit->text().toDouble();
    }

    simulationThread->setPlotVariables(plotVariables);
    simulationThread->logFMICalls = ui->logFMICallsCheckBox->isChecked();

    const QString logLevel = ui->logLevelComboBox->currentText();

    if (logLevel == "Warning") {
        simulationThread->logLevel = FMIWarning;
    } else if (logLevel == "Error") {
        simulationThread->logLevel = FMIError;
    } else {
        simulationThread->logLevel = FMIOK;
    }

    FMISimulationSettings* settings = simulationThread->settings;

    memset(settings, 0, sizeof(FMISimulationSettings));

    const QByteArray ba = unzipdir.toLocal8Bit();

    settings->unzipdir = strdup(ba.data());
    settings->modelDescription = modelDescription;

    if (interfaceTypeComboBox->currentText() == "Co-Simulation") {
        settings->interfaceType = FMICoSimulation;
        simulationThread->modelIdentifier = modelDescription->coSimulation->modelIdentifier;
    } else {
        settings->interfaceType = FMIModelExchange;
        simulationThread->modelIdentifier = modelDescription->modelExchange->modelIdentifier;
    }

    settings->visible                  = false;
    settings->loggingOn                = ui->debugLoggingCheckBox->isChecked();
    settings->tolerance                = ui->relativeToleranceLineEdit->text().toDouble();
    settings->nStartValues             = 0;
    settings->startVariables           = NULL;
    settings->startValues              = NULL;
    settings->tolerance                = ui->relativeToleranceLineEdit->text().toDouble();
    settings->startTime                = ui->startTimeLineEdit->text().toDouble();

    settings->outputInterval           = outputInterval;
    settings->stopTime                 = stopTime;
    settings->earlyReturnAllowed       = ui->earlyReturnAllowedCheckBox->isChecked();
    settings->eventModeUsed            = ui->eventModeUsedCheckBox->isChecked();
    settings->recordIntermediateValues = false;
    settings->initialFMUStateFile      = NULL;
    settings->finalFMUStateFile        = NULL;

    settings->nStartValues = startValues.count();
    settings->startVariables = (const FMIModelVariable**)calloc(settings->nStartValues, sizeof(FMIModelVariable*));
    settings->startValues = (const char**)calloc(settings->nStartValues, sizeof(char*));

    if (ui->solverComboBox->currentText() == "Euler") {
        settings->solverCreate = FMIEulerCreate;
        settings->solverFree   = FMIEulerFree;
        settings->solverStep   = FMIEulerStep;
        settings->solverReset  = FMIEulerReset;
    } else {
        settings->solverCreate = FMICVodeCreate;
        settings->solverFree   = FMICVodeFree;
        settings->solverStep   = FMICVodeStep;
        settings->solverReset  = FMICVodeReset;
    }

    size_t i = 0;

    for (auto [variable, value] : startValues.asKeyValueRange()) {
        settings->startVariables[i] = (FMIModelVariable*)variable;
        QByteArray buffer = value.toLocal8Bit();
        settings->startValues[i] = strdup(buffer.data());
        i++;
    }

    if (ui->inputCheckBox->isChecked()) {
        simulationThread->inputFilename = ui->inputLineEdit->text();
    } else {
        simulationThread->inputFilename = "";
    }

    ui->simulateAction->setIcon(QIcon(":/buttons/dark/stop.svg"));

    setCurrentPage(ui->plotPage);

    simulationThread->start();
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

    if (!simulationThread || !simulationThread->settings->recorder) {
        return;
    }

    std::sort(plotVariables.begin(), plotVariables.end(), [](const FMIModelVariable* a, const FMIModelVariable* b) {
        return QString(a->name) > QString(b->name);
    });

    const QString javaScript = PlotUtil::createPlot(
        simulationThread->settings->initialRecorder,
        simulationThread->settings->recorder,
        plotVariables,
        colorScheme);

    // qDebug().noquote() << javaScript;

    ui->plotWebEngineView->page()->runJavaScript(javaScript);
}

void MainWindow::unloadFMU() {

    variablesListModel->setModelDescription(nullptr);
    modelVariablesTreeModel->setModelDescription(nullptr);

    if (modelDescription) {
        FMIFreeModelDescription(modelDescription);
        modelDescription = nullptr;
    }

    if (buildDescription) {
        FMIFreeBuildDescription(buildDescription);
        buildDescription = nullptr;
    }

    if (!unzipdir.isEmpty()) {
        QByteArray bytes = unzipdir.toLocal8Bit();
        const char *cstr = bytes.data();
        FMIRemoveDirectory(cstr);
        unzipdir = "";
    }

    setWindowTitle("FMUSim " + QApplication::applicationVersion());

    if (!ui) {
        return;
    }

    ui->logPlainTextEdit->clear();

    setOptionalColumnsVisible(false);

    const QMap<AbstractModelVariablesModel::ColumnIndex, int> columnWidths = {
        {AbstractModelVariablesModel::NameColumn, 200},
        {AbstractModelVariablesModel::TypeColumn, 55},
        {AbstractModelVariablesModel::DimensionColumn, 75},
        {AbstractModelVariablesModel::ValueReferenceColumn, 100},
        {AbstractModelVariablesModel::InitialColumn, 70},
        {AbstractModelVariablesModel::CausalityColumn, 80},
        {AbstractModelVariablesModel::VariabilityColumn, 70},
        {AbstractModelVariablesModel::StartColumn, 70},
        {AbstractModelVariablesModel::NominalColumn, 70},
        {AbstractModelVariablesModel::MinColumn, 70},
        {AbstractModelVariablesModel::MaxColumn, 70},
        {AbstractModelVariablesModel::UnitColumn, 50},
        {AbstractModelVariablesModel::PlotColumn, 40}
    };

    for (auto i : columnWidths.asKeyValueRange()) {
        ui->modelVariablesListView->setColumnWidth(i.first, i.second);
        ui->modelVariablesTreeView->setColumnWidth(i.first, i.second);
    }

    ui->dockWidget->setHidden(true);

    ui->buildPlatformBinaryAction->setEnabled(false);
    ui->showInfoAction->setEnabled(false);
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

void MainWindow::runPlotScript(QString javaScript) {
    ui->plotWebEngineView->page()->runJavaScript(javaScript);
}

void MainWindow::addPlotVariable(const FMIModelVariable* variable) {

    plotVariables.append(variable);

    variablesListModel->setPlotVariables(&plotVariables);

    emit plotVariablesChanged(plotVariables);

    if (!simulationThread->isRunning()) {
        updatePlot();
    }
}

void MainWindow::removePlotVariable(const FMIModelVariable* variable) {

    plotVariables.removeAll(variable);

    variablesListModel->setPlotVariables(&plotVariables);

    emit plotVariablesChanged(plotVariables);

    if (!simulationThread->isRunning()) {
        updatePlot();
    }
}

void MainWindow::setOptionalColumnsVisible(bool visible) {

    for (AbstractModelVariablesModel::ColumnIndex i : {
            AbstractModelVariablesModel::TypeColumn,
            AbstractModelVariablesModel::DimensionColumn,
            AbstractModelVariablesModel::ValueReferenceColumn,
            AbstractModelVariablesModel::InitialColumn,
            AbstractModelVariablesModel::CausalityColumn,
            AbstractModelVariablesModel::VariabilityColumn,
            AbstractModelVariablesModel::NominalColumn,
            AbstractModelVariablesModel::MinColumn,
            AbstractModelVariablesModel::MaxColumn,
    }) {
        ui->modelVariablesListView->setColumnHidden(i, !visible);
        ui->modelVariablesTreeView->setColumnHidden(i, !visible);
    }
}

void MainWindow::simulationFinished() {

    ui->simulateAction->setIcon(QIcon(":/buttons/dark/play.svg"));

    ui->logPlainTextEdit->setPlainText(simulationThread->messages.join('\n'));

    updatePlot();

    ui->showPlotAction->setEnabled(true);

    if (simulationThread->status == FMIOK) {
        setCurrentPage(ui->plotPage);
    } else {
        setCurrentPage(ui->logPage);
    }
}

void MainWindow::buildPlatformBinary() {

    BuildPlatformBinaryDialog dialog(this);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    ui->logPlainTextEdit->clear();

    setCurrentPage(ui->logPage);

    buildPlatformBinaryProgressDialog->show();

    buildPlatformBinaryThread->unzipdir = unzipdir;
    buildPlatformBinaryThread->modelDescription = modelDescription;
    buildPlatformBinaryThread->buildDescription = buildDescription;
    buildPlatformBinaryThread->cmakeCommand = dialog.cmakeCommand();
    buildPlatformBinaryThread->cmakeGenerator = dialog.cmakeGenerator();
    buildPlatformBinaryThread->buildConfiguration = dialog.buildConfiguration();
    buildPlatformBinaryThread->compileWithWSL = dialog.compileWithWSL();
    buildPlatformBinaryThread->removeBuilDirectory = dialog.removeBuilDirectory();

    buildPlatformBinaryThread->start();
}

void MainWindow::showModelVariablesListView(bool show) {
    ui->modelVariablesStackedWidget->setCurrentWidget(show ? ui->listViewPage : ui->treeViewPage);
}
