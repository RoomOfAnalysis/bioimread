#include <QApplication>
#include <QImageReader>

#include "viewer.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // image allocation limit
    // or set environment variable QT_IMAGEIO_MAXALLOC=1024 to limit to 1GB
    // https://doc.qt.io/qt-6/qimagereader.html#setAllocationLimit
    QImageReader::setAllocationLimit(1024); // 1GB

    Viewer viewer;
    viewer.showMaximized();

    return app.exec();
}