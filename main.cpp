#include "gui.hpp"

#include <QApplication>
#include <QDebug>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Gui w;

    return app.exec();
}
