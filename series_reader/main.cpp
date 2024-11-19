#include "series_reader.hpp"

#include <QApplication>
#include <QFileInfo>
#include <QDebug>
#include <QMainWindow>
#include <QWidget>
#include <QLayout>
#include <QLabel>
#include <QPushButton>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QFileInfo info(argv[1]);
    if (!info.exists()) qDebug() << "meta.xml file not exist at:" << info.absoluteFilePath();
    auto meta_file_path = info.absoluteFilePath().toStdString();

    SeriesReader reader;
    if (!reader.open(meta_file_path.c_str()))
    {
        qCritical() << "can not read file:" << meta_file_path;
        return 0;
    }
    else
    {
        reader.setSeries(0);
        auto cur = reader.getImageCount() / 2;
        auto img = reader.getPlane(cur);
        QImage qimg = QImage((uchar*)img.data, img.cols, img.rows, img.step, QImage::Format_Grayscale8);

        QLabel label;
        label.setPixmap(QPixmap::fromImage(qimg));

        QLabel txt(QString::number(cur), &label);
        txt.setStyleSheet("color:yellow;font-size:18pt");

        QPushButton prev("prev"), next("next");
        QObject::connect(&prev, &QPushButton::pressed, [&reader, &txt, &label]() {
            if (auto cur = txt.text().toInt(); cur > 0)
            {
                cur--;
                auto img = reader.getPlane(cur);
                QImage qimg = QImage((uchar*)img.data, img.cols, img.rows, img.step, QImage::Format_Grayscale8);
                label.setPixmap(QPixmap::fromImage(qimg));
                txt.setText(QString::number(cur));
            }
        });
        QObject::connect(&next, &QPushButton::pressed, [&reader, &txt, &label]() {
            if (auto cur = txt.text().toInt(); cur < reader.getImageCount() - 1)
            {
                cur++;
                auto img = reader.getPlane(cur);
                QImage qimg = QImage((uchar*)img.data, img.cols, img.rows, img.step, QImage::Format_Grayscale8);
                label.setPixmap(QPixmap::fromImage(qimg));
                txt.setText(QString::number(cur));
            }
        });

        QWidget w;
        QVBoxLayout vl;
        vl.addWidget(&label);
        QHBoxLayout hl;
        hl.addWidget(&prev);
        hl.addWidget(&next);
        vl.addLayout(&hl);
        w.setLayout(&vl);

        QMainWindow m;
        m.setWindowTitle("series reader");
        m.setCentralWidget(&w);
        m.showMaximized();

        return app.exec();
    }

    return 0;
}