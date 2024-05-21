#include "MainWindow.h"
#include "./ui_MainWindow.h"

extern "C" {
#include "FMIModelDescription.h"
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    FMIModelDescription* modelDescription = FMIReadModelDescription("C:\\Users\\tsr2\\Downloads\\Reference-FMUs-0.0.31\\3.0\\BouncingBall\\modelDescription.xml");

    ui->pushButton->setText(modelDescription->modelName);

    FMIFreeModelDescription(modelDescription);
}

MainWindow::~MainWindow()
{
    delete ui;
}
