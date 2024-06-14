#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::PlaceholderText,QColor(127,127,127));
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(164, 166, 168));
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(164, 166, 168));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(164, 166, 168));
    darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(164, 166, 168));
    darkPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(68, 68, 68));
    darkPalette.setColor(QPalette::Disabled, QPalette::Window, QColor(68, 68, 68));
    darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(68, 68, 68));

    a.setStyle("Fusion");
    // a.setStyle("Windows");
    // a.setStyle("WindowsVista");

    MainWindow *w = new MainWindow();
    w->show();

    for (qsizetype i = 1; i < a.arguments().length(); i++) {

        if (i > 1) {
            w = new MainWindow();
            w->show();
        }

        w->loadFMU(a.arguments().at(i));
    }

    // w.setPalette(darkPalette);

    return a.exec();
}
