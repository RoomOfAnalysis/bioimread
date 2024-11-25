#include "image5dviewer.hpp"
#include "ui_image5dviewer.h"

#include <QFileDialog>
#include <QMimeDatabase>
#include <QImageReader>
#include <QMovie>

#include "bfwrapper/reader.hpp"
#include "utils/plane2qimg.hpp"

Image5DViewer::Image5DViewer(QWidget* parent): QWidget(parent), ui(new Ui::Image5DViewer)
{
    ui->setupUi(this);

    connect(ui->slider_s, &QSlider::valueChanged, [this](int) { update(true); });
    connect(ui->slider_z, &QSlider::valueChanged, [this](int) { update(true); });
    connect(ui->slider_c, &QSlider::valueChanged, [this](int) { update(true); });
    connect(ui->slider_t, &QSlider::valueChanged, [this](int) { update(true); });

    connect(ui->btn, &QPushButton::pressed, [this] { openFile(); });

    connect(ui->s_sbox, &QSpinBox::valueChanged, [this](int) { update(false); });
    connect(ui->z_sbox, &QSpinBox::valueChanged, [this](int) { update(false); });
    connect(ui->c_sbox, &QSpinBox::valueChanged, [this](int) { update(false); });
    connect(ui->t_sbox, &QSpinBox::valueChanged, [this](int) { update(false); });

    reader = new Reader;
}

Image5DViewer::~Image5DViewer()
{
    delete reader;
}

void Image5DViewer::openFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Image"), "", tr("*;;*.lsm;; *.czi;; *.ome.tiff"));
    if (filePath.isEmpty()) return;

    reader->close();

    QString format;
    QMimeDatabase mime_db;
    auto mime_type = mime_db.mimeTypeForFile(filePath, QMimeDatabase::MatchContent);
    auto mime_name = mime_type.name().toUtf8();
    if (mime_name == "image/jpeg")
        format = "jpg";
    else if (mime_name == "image/png")
        format = "png";
    else if (mime_name == "image/gif")
        format = "gif";
    else if (mime_name == "image/webp" ||
             (mime_name == "audio/x-riff" && QFileInfo(filePath).suffix().toLower().toUtf8() == "webp"))
        format = "webp";
    else if (mime_name == "image/jxl")
        format = "jxl";
    else if (mime_name == "image/bmp")
        format = "bmp";
    // handle plain image with qt
    if (!format.isEmpty())
    {
        if (format == "gif")
        {
            QMovie* movie = new QMovie(filePath);
            if (movie->isValid())
            {
                movie->setScaledSize(ui->viewer->size());
                ui->viewer->setMovie(movie);
                movie->start();
                resetSliders();
                return;
            }
        }
        QImageReader qreader(filePath, format.toUtf8());
        if (qreader.canRead())
        {
            ui->viewer->setPixmap(QPixmap::fromImage(
                qreader.read().scaled(ui->viewer->width(), ui->viewer->height(), Qt::KeepAspectRatio)));
            resetSliders();
            return;
        }
    }

    auto succeed = reader->open(filePath.toStdString());
    ui->slider_s->setEnabled(succeed);
    ui->slider_z->setEnabled(succeed);
    ui->slider_c->setEnabled(succeed);
    ui->slider_t->setEnabled(succeed);
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

    auto qimg = readPlaneToQimage(*reader, plane);
    ui->viewer->setPixmap(
        QPixmap::fromImage(qimg.scaled(ui->viewer->width(), ui->viewer->height(), Qt::KeepAspectRatio)));
}

void Image5DViewer::resetSliders()
{
    const QSignalBlocker bss(ui->slider_s);
    const QSignalBlocker bsz(ui->slider_z);
    const QSignalBlocker bsc(ui->slider_c);
    const QSignalBlocker bst(ui->slider_t);

    const QSignalBlocker bsbs(ui->s_sbox);
    const QSignalBlocker bsbz(ui->z_sbox);
    const QSignalBlocker bsbc(ui->c_sbox);
    const QSignalBlocker bsbt(ui->t_sbox);

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
