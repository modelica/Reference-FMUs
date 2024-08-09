#include "BuildPlatformBinaryDialog.h"
#include "ui_BuildPlatformBinaryDialog.h"

BuildPlatformBinaryDialog::BuildPlatformBinaryDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BuildPlatformBinaryDialog) {
    ui->setupUi(this);
}

BuildPlatformBinaryDialog::~BuildPlatformBinaryDialog() {
    delete ui;
}

QString BuildPlatformBinaryDialog::cmakeCommand() const {
    return ui->cmakeComandLineEdit->text();
}

QString BuildPlatformBinaryDialog::cmakeGenerator() const {
    return ui->cmakeGeneratorComboBox->currentText();
}

QString BuildPlatformBinaryDialog::buildConfiguration() const {
    return ui->buildConfigurationComboBox->currentText();
}

bool BuildPlatformBinaryDialog::compileWithWSL() const {
    return ui->compileWithWSLCheckBox->isChecked();
}

bool BuildPlatformBinaryDialog::removeBuilDirectory() const {
    return ui->removeBuildDirectoryCheckBox->isChecked();
}
