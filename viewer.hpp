#pragma once

#include <QWidget>

namespace Ui
{
    class Viewer;
}

class Viewer final: public QWidget
{
    Q_OBJECT
public:
    explicit Viewer(QWidget* parent = nullptr);
    ~Viewer();

private:
    Ui::Viewer* ui;
};