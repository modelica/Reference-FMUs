#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QStyleHints>


int main(int argc, char *argv[]) {

    QApplication app(argc, argv);
    QApplication::setApplicationVersion("0.0.31");
    QApplication::setOrganizationName("Modelica Association");
    QApplication::setApplicationName("FMUSim");

    app.setStyle("Fusion");

    QFont font = app.font();
    font.setPointSize(10);
    app.setFont(font);

    MainWindow *window = new MainWindow();
    window->show();

    for (qsizetype i = 1; i < app.arguments().length(); i++) {

        if (i > 1) {
            window = new MainWindow();
            window->show();
        }

        window->loadFMU(app.arguments().at(i));
    }

    return app.exec();
}
