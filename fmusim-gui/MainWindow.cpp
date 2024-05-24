#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QStringListModel>
#include "ModelVariablesItemModel.h"

extern "C" {
#include "FMIModelDescription.h"
#include "FMIZip.h"
}

#define FMI_PATH_MAX 4096

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    const char* unzipdir = FMICreateTemporaryDirectory();

    char modelDescriptionPath[FMI_PATH_MAX] = "";

    //char platformBinaryPath[FMI_PATH_MAX] = "";

    int status = FMIExtractArchive("C:\\Users\\tsr2\\Downloads\\Reference-FMUs-0.0.31\\3.0\\BouncingBall.fmu", unzipdir);

    FMIPathAppend(modelDescriptionPath, unzipdir);
    FMIPathAppend(modelDescriptionPath, "modelDescription.xml");

    FMIModelDescription* modelDescription = FMIReadModelDescription(modelDescriptionPath); //"C:\\Users\\tsr2\\Downloads\\Reference-FMUs-0.0.31\\3.0\\BouncingBall\\modelDescription.xml");

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

    ui->variablesLabel->setText(QString::number(modelDescription->nModelVariables));

    ui->generationDateLabel->setText(modelDescription->generationDate);
    ui->generationToolLabel->setText(modelDescription->generationTool);
    ui->descriptionLabel->setText(modelDescription->description);

    ModelVariablesItemModel* model = new ModelVariablesItemModel(modelDescription, this);

    // QStringListModel *model = new QStringListModel();
    // QStringList list;

    // for (size_t i = 0; i < modelDescription->nModelVariables; i++) {
    //     list << modelDescription->modelVariables[i].name;
    // }

    // model->setStringList(list);

    ui->listView->setModel(model);


    if (unzipdir) {
        FMIRemoveDirectory(unzipdir);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    // TODO: FMIFreeModelDescription(modelDescription);
}
