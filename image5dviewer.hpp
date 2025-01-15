#pragma once

#include <QWidget>

class Reader;

namespace Ui
{
    class Image5DViewer;
}

class Image5DViewer: public QWidget
{
    Q_OBJECT
public:
    explicit Image5DViewer(QWidget* parent = nullptr);
    ~Image5DViewer();

signals:
    void fileOpened(QString const& path, QSize sz, int depth, QString const& xml);
    void seriesChanged(int series_no);

private:
    void openFile();
    void update(bool);
    void resetSliders();

private:
    Ui::Image5DViewer* ui;
    Reader* reader = nullptr;
    QImage curr_img{};
};