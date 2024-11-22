#include <QApplication>

#include "image5dviewer.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    Image5DViewer viewer;
    viewer.show();

    return app.exec();
}