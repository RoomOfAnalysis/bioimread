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
    void fileOpened(QString const& path, QString const& xml);

private:
    void openFile();
    void update(bool);
    void resetSliders();

private:
    Ui::Image5DViewer* ui;
    Reader* reader = nullptr;
    QImage curr_img{};
};