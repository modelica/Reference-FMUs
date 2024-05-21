#include "MainWindow.h"
#include "./ui_MainWindow.h"

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

    ui->pushButton->setText(modelDescription->modelName);

    FMIFreeModelDescription(modelDescription);

    if (unzipdir) {
        FMIRemoveDirectory(unzipdir);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
