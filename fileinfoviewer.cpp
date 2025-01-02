#include "fileinfoviewer.hpp"

#include <QLayout>
#include <QStyleOption>
#include <QPainter>
#include <QSpacerItem>
#include <QFileInfo>
#include <QDebug>
#include <QTextBrowser>

KVItem::KVItem(QWidget* parent): QWidget(parent)
{
    m_value_label.setContentsMargins(3, 0, 0, 0);
    m_value_label.setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_value_label.setCursor(Qt::IBeamCursor);

    m_value_label.setWordWrap(true);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(9, 0, 9, 0);
    layout->setSpacing(0);
    layout->addWidget(&m_key_label);
    layout->addWidget(&m_value_label);
    setLayout(layout);
}

void KVItem::setKV(QString key, QString value)
{
    m_key_label.setText(std::move(key));
    m_value_label.setText(std::move(value).split(QString()).join("\u200b"));
};

void KVItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

FileInfoViewer::FileInfoViewer(QWidget* parent): QWidget(parent)
{
    m_path_item = new KVItem(this);
    m_size_item = new KVItem(this);
    m_isdir_item = new KVItem(this);
    m_lastmodified_item = new KVItem(this);
    m_extra = new QTextBrowser(this);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(9, 0, 9, 0);
    setLayout(layout);

    layout->addWidget(m_path_item);
    layout->addWidget(m_isdir_item);
    layout->addWidget(m_size_item);
    layout->addWidget(m_lastmodified_item);
    layout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Fixed));
    layout->addWidget(m_extra);
}

FileInfoViewer::~FileInfoViewer() = default;

void FileInfoViewer::setPath(QString const& path)
{
    m_fi.setFile(path);
    if (path.isEmpty() || !m_fi.exists() || (!m_fi.isFile() && !m_fi.isDir()))
    {
        qWarning() << "FileInfoViewer::setPath: invalid path" << path;

        m_path_item->setKV("Path:", path);
        m_size_item->setKV("Size:", "invalid path");
        m_isdir_item->setKV("Is Dir:", "invalid path");
        m_lastmodified_item->setKV("Last Modified:", "invalid path");
        return;
    }

    m_path_item->setKV("Path:", m_fi.absoluteFilePath());
    m_isdir_item->setKV("Is Dir:", QVariant(m_fi.isDir()).toString());
    m_size_item->setKV("Size:", locale().formattedDataSize(m_fi.size()));
    m_lastmodified_item->setKV("Last Modified:", m_fi.lastModified().toString());
}

QString FileInfoViewer::getFileName() const
{
    return m_fi.fileName();
}

void FileInfoViewer::setExtra(QString const& xml_str)
{
    // TODO: parse XML into a pretty format
    m_extra->setMarkdown(xml_str);
}
