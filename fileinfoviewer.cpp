#include "fileinfoviewer.hpp"

#include <QLayout>
#include <QStyleOption>
#include <QPainter>
#include <QSpacerItem>
#include <QFileInfo>
#include <QDebug>
#include <QTextBrowser>
#include <QDomDocument>

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
    m_xml_str = xml_str;
    m_series_no = -1;
    m_instrument_strs.clear();

    // https://ome-model.readthedocs.io/en/stable/developers/model-overview.html
    QDomDocument doc("meta");
    doc.setContent(m_xml_str);
    QDomElement topElement = doc.documentElement();
    QDomNode domNode = topElement.firstChild();
    while (!domNode.isNull())
    {
        QDomElement domElement = domNode.toElement();
        if (!domElement.isNull())
        {
            // Experimenter / Instrument / Image / StructuredAnnotations / ...
            // qDebug() << domElement.tagName();
            if (domElement.tagName() == "Instrument")
            {
                QString intrument_str = parse_node(domElement.firstChild());
                m_instrument_strs.append(intrument_str);
            }
        }
        domNode = domNode.nextSibling();
    }

    setExtra(0);
}

void FileInfoViewer::setExtra(int series_no)
{
    if (m_series_no == series_no) return;
    m_series_no = series_no;

    m_extra->setText("");
    if (m_xml_str.isEmpty()) return;

    // https://ome-model.readthedocs.io/en/stable/developers/model-overview.html
    QDomDocument doc("meta");
    doc.setContent(m_xml_str);
    QDomElement topElement = doc.documentElement();
    QDomNode domNode = topElement.firstChild();
    while (!domNode.isNull())
    {
        QDomElement domElement = domNode.toElement();
        if (!domElement.isNull())
        {
            if (domElement.tagName() == "Image" && domElement.attribute("ID") == QString("Image:%1").arg(m_series_no))
            {
                // <InstrumentRef ID="Instrument:0"/>
                m_extra->setText(
                    m_instrument_strs
                        [domElement.namedItem("InstrumentRef").toElement().attribute("ID").split(":")[1].toInt()]);
                break;
            }
        }
        domNode = domNode.nextSibling();
    }
}

QString FileInfoViewer::parse_node(QDomNode node, int indent)
{
    QString str{};
    while (!node.isNull())
    {
        QDomElement element = node.toElement();
        if (!element.isNull() && element.hasAttributes())
        {
            str.append(QString(indent, QChar::Space) + element.tagName() + ":\n");
            QDomNamedNodeMap map = element.attributes();
            for (auto i = 0; i < map.length(); i++)
                if (!(map.item(i).isNull()))
                {
                    if (QDomAttr attr = map.item(i).toAttr(); !attr.isNull())
                        str.append(QString("%1%2 = %3\n")
                                       .arg(QString(indent + 2, QChar::Space))
                                       .arg(attr.name())
                                       .arg(attr.value()));
                }
        }
        if (element.hasChildNodes())
        {
            auto childNodes = element.childNodes();
            for (auto i = 0; i < childNodes.length(); i++)
                str += parse_node(childNodes.at(i), indent + 4);
        }

        node = node.nextSibling();
    }
    return str;
}
