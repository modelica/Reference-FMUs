#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QStyleHints>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationVersion("0.0.31");
    QApplication::setOrganizationName("Modelica");
    QApplication::setApplicationName("FMUSim");

    // QStyleHints *styleHints = QGuiApplication::styleHints();

    // qDebug() << styleHints->colorScheme();

    // qDebug() << QIcon::themeSearchPaths();

    // QStringList themeSearchPaths;

    // themeSearchPaths << ":/buttons";

    // QIcon::setThemeSearchPaths(themeSearchPaths);

    // qDebug() << QIcon::themeSearchPaths();

    // QIcon::setThemeName("dark");

    // QPalette darkPalette;
    // darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    // darkPalette.setColor(QPalette::WindowText, Qt::white);
    // darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    // darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    // darkPalette.setColor(QPalette::ToolTipBase, QColor(53, 53, 53));
    // darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    // darkPalette.setColor(QPalette::Text, Qt::white);
    // darkPalette.setColor(QPalette::PlaceholderText,QColor(127,127,127));
    // darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    // darkPalette.setColor(QPalette::ButtonText, Qt::white);
    // darkPalette.setColor(QPalette::BrightText, Qt::red);
    // darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    // darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    // darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    // darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(164, 166, 168));
    // darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(164, 166, 168));
    // darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(164, 166, 168));
    // darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(164, 166, 168));
    // darkPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(68, 68, 68));
    // darkPalette.setColor(QPalette::Disabled, QPalette::Window, QColor(68, 68, 68));
    // darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(68, 68, 68));

    app.setStyle("Fusion");
    // a.setStyle("Windows");
    // a.setStyle("WindowsVista");

    QFont f = app.font();

    qDebug() << f;

    //f.setFamily("Monaco");
    f.setPointSize(10);

    app.setFont(f);

    MainWindow *w = new MainWindow();
    // w->setColorScheme(Qt::ColorScheme::Dark);
    w->show();

    for (qsizetype i = 1; i < app.arguments().length(); i++) {

        if (i > 1) {
            w = new MainWindow();
            // w->setColorScheme(Qt::ColorScheme::Dark);
            w->show();
        }

        w->loadFMU(app.arguments().at(i));
    }

    // w.setPalette(darkPalette);

    return app.exec();
}
