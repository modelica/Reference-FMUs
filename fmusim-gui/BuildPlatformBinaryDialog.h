#ifndef BUILDPLATFORMBINARYDIALOG_H
#define BUILDPLATFORMBINARYDIALOG_H

#include <QDialog>

namespace Ui {
class BuildPlatformBinaryDialog;
}

class BuildPlatformBinaryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BuildPlatformBinaryDialog(QWidget *parent = nullptr);
    ~BuildPlatformBinaryDialog();

    QString cmakeCommand() const;
    QString cmakeGenerator() const;
    QString buildConfiguration() const;
    bool removeBuilDirectory() const;

private:
    Ui::BuildPlatformBinaryDialog *ui;
};

#endif // BUILDPLATFORMBINARYDIALOG_H
