#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

extern "C" {
#include "FMIModelDescription.h"
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadFMU(const QString &filename);

private:
    Ui::MainWindow *ui;
    QFileSystemModel filesModel;
    FMIModelDescription* modelDescription = nullptr;
    QString unzipdir;

    void setCurrentPage(QWidget *page);

private slots:
    void openFileInDefaultApplication(const QModelIndex &index);

};
#endif // MAINWINDOW_H
