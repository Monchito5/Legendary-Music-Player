#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    MainWindow w;
    w.show();
    
    // Establecer estilo moderno
    app.setStyle(QStyleFactory::create("Fusion"));

    // Cargar hoja de estilo
    QFile styleFile(":/styles/blue.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = styleFile.readAll();
        app.setStyleSheet(style);
        styleFile.close();
    }

    return app.exec();
}
