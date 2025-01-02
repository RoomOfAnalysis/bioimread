#include <QApplication>

#include "viewer.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    Viewer viewer;
    viewer.showMaximized();

    return app.exec();
}