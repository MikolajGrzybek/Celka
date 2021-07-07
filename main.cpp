#include "gui.hpp"

#include <QApplication>

#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Gui w;
    w.show();
    w.setWindowState(Qt::WindowFullScreen);

    return app.exec();
}
