#include "gui.hpp"

#include <QCoreApplication>
#include <QDebug>


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Gui w;

    return app.exec();
}
