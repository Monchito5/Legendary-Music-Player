#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Establecer estilo moderno
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Cargar hoja de estilo
    QFile styleFile(":/styles/light.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = styleFile.readAll();
        app.setStyleSheet(style);
        styleFile.close();
    }
    
    MainWindow w;
    w.show();
    
    return app.exec();
}
