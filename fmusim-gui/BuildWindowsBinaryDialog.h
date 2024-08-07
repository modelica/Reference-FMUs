#ifndef BUILDWINDOWSBINARYDIALOG_H
#define BUILDWINDOWSBINARYDIALOG_H

#include <QDialog>

namespace Ui {
class BuildWindowsBinaryDialog;
}

class BuildWindowsBinaryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BuildWindowsBinaryDialog(QWidget *parent = nullptr);
    ~BuildWindowsBinaryDialog();

    QString cmakeCommand() const;
    QString cmakeGenerator() const;
    QString buildConfiguration() const;
    bool removeBuilDirectory() const;

private:
    Ui::BuildWindowsBinaryDialog *ui;
};

#endif // BUILDWINDOWSBINARYDIALOG_H
