#include "BuildWindowsBinaryDialog.h"
#include "ui_BuildWindowsBinaryDialog.h"

BuildWindowsBinaryDialog::BuildWindowsBinaryDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BuildWindowsBinaryDialog) {
    ui->setupUi(this);
}

BuildWindowsBinaryDialog::~BuildWindowsBinaryDialog() {
    delete ui;
}

QString BuildWindowsBinaryDialog::cmakeCommand() const {
    return ui->cmakeComandLineEdit->text();
}

QString BuildWindowsBinaryDialog::cmakeGenerator() const {
    return ui->cmakeGeneratorComboBox->currentText();
}

QString BuildWindowsBinaryDialog::buildConfiguration() const {
    return ui->buildConfigurationComboBox->currentText();
}

bool BuildWindowsBinaryDialog::removeBuilDirectory() const {
    return ui->removeBuildDirectoryCheckBox->isChecked();
}
