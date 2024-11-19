#include <QApplication>
#include <QFileInfo>
#include <QDebug>
#include <QMainWindow>
#include <QWidget>
#include <QLayout>
#include <QLabel>
#include <QPushButton>

#include "bfwrapper/reader.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QFileInfo info(argv[1]);
    if (!info.exists()) qDebug() << "image file not exist at:" << info.absoluteFilePath();
    auto img_file_path = info.absoluteFilePath().toStdString();

    Reader reader;
    if (!reader.open(img_file_path.c_str()))
    {
        qCritical() << "can not read file:" << img_file_path;
        return 0;
    }
    else
    {
        reader.setSeries(0);
        auto cur = reader.getImageCount() / 2;
        auto img = reader.getPlane(cur);
        auto qformat = img.channels() == 1 ?
                           (img.type() == CV_16U ? QImage::Format_Grayscale16 : QImage::Format_Grayscale8) :
                           QImage::Format_BGR888;
        QImage qimg = QImage((uchar*)img.data, img.cols, img.rows, img.step, qformat);

        QLabel label;
        label.setPixmap(QPixmap::fromImage(qimg));

        QLabel txt(QString::number(cur), &label);
        txt.setStyleSheet("color:yellow;font-size:18pt");

        QPushButton prev("prev"), next("next");
        QObject::connect(&prev, &QPushButton::pressed, [&reader, &txt, &label, qformat]() {
            if (auto cur = txt.text().toInt(); cur > 0)
            {
                cur--;
                auto img = reader.getPlane(cur);
                QImage qimg = QImage((uchar*)img.data, img.cols, img.rows, img.step, qformat);
                label.setPixmap(QPixmap::fromImage(qimg));
                txt.setText(QString::number(cur));
            }
        });
        QObject::connect(&next, &QPushButton::pressed, [&reader, &txt, &label, qformat]() {
            if (auto cur = txt.text().toInt(); cur < reader.getImageCount() - 1)
            {
                cur++;
                auto img = reader.getPlane(cur);
                QImage qimg = QImage((uchar*)img.data, img.cols, img.rows, img.step, qformat);
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
        m.setWindowTitle("bioimread");
        m.setCentralWidget(&w);
        m.showMaximized();

        return app.exec();
    }
}