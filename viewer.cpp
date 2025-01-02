#include "viewer.hpp"
#include "ui_viewer.h"

Viewer::Viewer(QWidget* parent): QWidget(parent), ui(new Ui::Viewer)
{
    ui->setupUi(this);

    connect(ui->image5dviewer, &Image5DViewer::fileOpened, [this](QString const& path, QString const& xml) {
        ui->fileinfoviewer->setPath(path);
        if (!xml.isEmpty()) ui->fileinfoviewer->setExtra(xml);
    });
}

Viewer::~Viewer()
{
    delete ui;
}
