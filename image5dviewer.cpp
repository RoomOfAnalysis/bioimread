#include "image5dviewer.hpp"
#include "ui_image5dviewer.h"

#include <QFileDialog>
#include <QImageReader>
#include <QMovie>

#include "bfwrapper/reader.hpp"
#include "utils/plane2qimg.hpp"

Image5DViewer::Image5DViewer(QWidget* parent): QWidget(parent), ui(new Ui::Image5DViewer)
{
    ui->setupUi(this);

    connect(ui->slider_s, &QSlider::valueChanged, [this](int) {
        update(true);
        seriesChanged(ui->slider_s->value());
    });
    connect(ui->slider_z, &QSlider::valueChanged, [this](int) { update(true); });
    connect(ui->slider_c, &QSlider::valueChanged, [this](int) { update(true); });
    connect(ui->slider_t, &QSlider::valueChanged, [this](int) { update(true); });

    connect(ui->btn, &QPushButton::pressed, [this] { openFile(); });

    connect(ui->s_sbox, &QSpinBox::valueChanged, [this](int) {
        update(false);
        seriesChanged(ui->slider_s->value());
    });
    connect(ui->z_sbox, &QSpinBox::valueChanged, [this](int) { update(false); });
    connect(ui->c_sbox, &QSpinBox::valueChanged, [this](int) { update(false); });
    connect(ui->t_sbox, &QSpinBox::valueChanged, [this](int) { update(false); });
}

Image5DViewer::~Image5DViewer()
{
    delete ui;
    delete reader;
}

void Image5DViewer::openFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Image"), "", tr("*;;*.lsm;; *.czi;; *.ome.tiff"));
    if (filePath.isEmpty()) return;

    if (reader) reader->close();

    // FIXME: have to always use new reader to obtain current file's XML...
    delete reader;
    reader = new Reader;

    // handle plain image with qt
    // TODO: mime type seems more reliable than file extension
    if (auto format = filePath.split(".").last().toLower(); QImageReader::supportedImageFormats().contains(format))
    {
        QImageReader qreader(filePath);
        auto canRead = qreader.canRead();
        // handle gif with QImageReader since bioformats read gif out of shape...
        // handle single page tiff with QImageReader instead of bioformats
        if (format != "gif") canRead &= (qreader.imageCount() == 1);
        if (canRead)
        {
            if (format == "gif")
            {
                if (QMovie movie = QMovie(filePath); movie.isValid())
                {
                    movie.setScaledSize(ui->viewer->size());
                    movie.start();
                    movie.jumpToFrame(movie.frameCount() / 2);
                    curr_img = movie.currentImage();
                }
                // TODO: set timepoint with `ui->slider_t`
            }
            else
                curr_img = qreader.read();
            // in some case, QImageReader may fail to read, then let bioformats handle it
            if (!curr_img.isNull())
            {
                ui->viewer->loadImage(curr_img);
                resetSliders();

                emit fileOpened(filePath, curr_img.size(), curr_img.bitPlaneCount(), {});

                return;
            }
        }
    }

    // https://docs.openmicroscopy.org/bio-formats/6.9.0/developers/wsi.html
    reader->setFlattenedResolutions(false);
    auto succeed = reader->open(filePath.toStdString());
    ui->slider_s->setEnabled(succeed);
    ui->slider_z->setEnabled(succeed);
    ui->slider_c->setEnabled(succeed);
    ui->slider_t->setEnabled(succeed);
    ui->s_sbox->setEnabled(succeed);
    ui->z_sbox->setEnabled(succeed);
    ui->c_sbox->setEnabled(succeed);
    ui->t_sbox->setEnabled(succeed);
    if (succeed)
    {
        const QSignalBlocker bss(ui->slider_s);
        const QSignalBlocker bsz(ui->slider_z);
        const QSignalBlocker bsc(ui->slider_c);
        const QSignalBlocker bst(ui->slider_t);

        const QSignalBlocker bsbs(ui->s_sbox);
        const QSignalBlocker bsbz(ui->z_sbox);
        const QSignalBlocker bsbc(ui->c_sbox);
        const QSignalBlocker bsbt(ui->t_sbox);

        reader->setSeries(0);

        ui->slider_s->setMaximum(reader->getSeriesCount() - 1);
        ui->slider_z->setMaximum(reader->getSizeZ() - 1);
        ui->slider_c->setMaximum(reader->getSizeC() - 1);
        ui->slider_t->setMaximum(reader->getSizeT() - 1);

        ui->slider_s->setValue(0);
        ui->slider_z->setValue(0);
        ui->slider_c->setValue(0);
        ui->slider_t->setValue(0);

        ui->s_sbox->setMaximum(reader->getSeriesCount() - 1);
        ui->z_sbox->setMaximum(reader->getSizeZ() - 1);
        ui->c_sbox->setMaximum(reader->getSizeC() - 1);
        ui->t_sbox->setMaximum(reader->getSizeT() - 1);

        ui->s_sbox->setValue(0);
        ui->z_sbox->setValue(0);
        ui->c_sbox->setValue(0);
        ui->t_sbox->setValue(0);

        update(true);

        emit fileOpened(filePath, curr_img.size(), reader->getBitsPerPixel(),
                        QString::fromStdString(reader->getMetaXML()));
    }
}

void Image5DViewer::update(bool is_slider)
{
    int s = 0, z = 0, c = 0, t = 0;

    const QSignalBlocker bss(ui->slider_s);
    const QSignalBlocker bsz(ui->slider_z);
    const QSignalBlocker bsc(ui->slider_c);
    const QSignalBlocker bst(ui->slider_t);

    const QSignalBlocker bsbs(ui->s_sbox);
    const QSignalBlocker bsbz(ui->z_sbox);
    const QSignalBlocker bsbc(ui->c_sbox);
    const QSignalBlocker bsbt(ui->t_sbox);

    if (!is_slider)
    {
        s = ui->s_sbox->value();
        z = ui->z_sbox->value();
        c = ui->c_sbox->value();
        t = ui->t_sbox->value();
        ui->slider_s->setValue(s);
        ui->slider_z->setValue(z);
        ui->slider_c->setValue(c);
        ui->slider_t->setValue(t);
    }
    else
    {
        s = ui->slider_s->value();
        z = ui->slider_z->value();
        c = ui->slider_c->value();
        t = ui->slider_t->value();
        ui->s_sbox->setValue(s);
        ui->z_sbox->setValue(z);
        ui->c_sbox->setValue(c);
        ui->t_sbox->setValue(t);
    }

    reader->setSeries(s);

    auto plane = reader->getPlaneIndex(z, c, t);
    ui->status->setText(QString("plane: %1 (s = %2, z = %3, c = %4, t = %5)").arg(plane).arg(s).arg(z).arg(c).arg(t));

    auto x_size = reader->getSizeX();
    auto y_size = reader->getSizeY();
    if (auto size = (long long)x_size * y_size * (reader->getRGBChannelCount()) * (reader->getBytesPerPixel());
        size < 0 || size > 2147483647) // 2GB
    {
        // TODO: read higher resolution in tiles
        //auto tile_x_size = reader->getOptimalTileWidth();
        //auto tile_y_size = reader->getOptimalTileHeight();
        //auto tiles_x = x_size / tile_x_size / 2;
        //auto tiles_y = y_size / tile_y_size / 2;
        //qDebug() << "image size(" << x_size << "," << y_size << ") is too large, load tile size(" << tile_x_size << ","
        //         << tile_y_size << ")"
        //         << "at [" << tiles_x << "," << tiles_y << "] instead ";
        //curr_img = readPlaneTileToQimage(*reader, plane, tiles_x, tiles_y);

        auto levels = reader->getResolutionCount();
        reader->setResolution(levels - 1); // set to the lowest resolution
        qDebug() << "levels:" << levels << ", size:" << reader->getSizeX() << reader->getSizeY();
        curr_img = readPlaneToQimage(*reader, plane);
    }
    else
        curr_img = readPlaneToQimage(*reader, plane);
    ui->viewer->loadImage(curr_img);
}

void Image5DViewer::resetSliders()
{
    ui->slider_s->setEnabled(false);
    ui->slider_z->setEnabled(false);
    ui->slider_c->setEnabled(false);
    ui->slider_t->setEnabled(false);

    ui->s_sbox->setEnabled(false);
    ui->z_sbox->setEnabled(false);
    ui->c_sbox->setEnabled(false);
    ui->t_sbox->setEnabled(false);

    ui->slider_s->setMaximum(0);
    ui->slider_z->setMaximum(0);
    ui->slider_c->setMaximum(0);
    ui->slider_t->setMaximum(0);

    ui->slider_s->setValue(0);
    ui->slider_z->setValue(0);
    ui->slider_c->setValue(0);
    ui->slider_t->setValue(0);

    ui->s_sbox->setMaximum(0);
    ui->z_sbox->setMaximum(0);
    ui->c_sbox->setMaximum(0);
    ui->t_sbox->setMaximum(0);

    ui->s_sbox->setValue(0);
    ui->z_sbox->setValue(0);
    ui->c_sbox->setValue(0);
    ui->t_sbox->setValue(0);

    ui->status->setText("Plain Image");
}
