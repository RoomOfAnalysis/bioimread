#include "series_reader.hpp"

#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QDebug>

#include <opencv2/imgcodecs.hpp>

static double lengthUnitTransfer(QString const& unit)
{
    double value = 1.0;
    if (unit == "nm")
        return (value * 1e-6);
    else if (unit == "um" || unit == u"µm")
        return (value * 1e-3);
    else if (unit == "mm")
        return value;
    else if (unit == "cm")
        return (value * 1e1);
    else if (unit == "dm")
        return (value * 1e2);
    else if (unit == "m")
        return (value * 1e3);
    qCritical() << "out of length range with unit:" << unit;
    return value;
}

static double timeUnitTransfer(QString const& unit)
{
    double value = 1.0;
    if (unit == "h")
        return (value * 3600);
    else if (unit == "min")
        return (value * 60);
    else if (unit == "ms")
        return (value * 1e-3);
    else if (unit == "s")
        return value;
    else if (unit == "us" || unit == u"µs")
        return (value * 1e-6);
    else if (unit == "ns")
        return (value * 1e-9);
    qCritical() << "out of time range with unit:" << unit;
    return value;
}

SeriesReader::SeriesReader() = default;

SeriesReader::~SeriesReader()
{
    close();
}

bool SeriesReader::open(std::string metaXmlFilePath)
{
    auto file_path = QString::fromStdString(metaXmlFilePath);
    // opencv xml reader is quite limited... https://github.com/opencv/opencv/issues/9868
    // so have to use other generic xml parser lib
    m_doc = new QDomDocument("meta");
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical() << "can not open file:" << file_path;
        return false;
    }
    if (!m_doc->setContent(&file))
    {
        qCritical() << "can not read file:" << file_path;
        file.close();
        return false;
    }
    file.close();
    m_folder = QFileInfo(file_path).absoluteDir().absolutePath().toStdString();

    auto images = m_doc->elementsByTagName("Image");
    m_meta.series_count = images.size();

    return true;
}

void SeriesReader::close()
{
    delete m_doc;
}

void SeriesReader::setSeries(int no)
{
    static QString typeStr[9]{"int8", "uint8", "int16", "uint16", "int32", "uint32", "float", "double", "bit"};
    static int type[9]{CV_8S, CV_8U, CV_16S, CV_16U, CV_32S, CV_32S, CV_32F, CV_64F, CV_8S};
    static int bytesPerPixel[9]{1, 1, 2, 2, 4, 4, 4, 8, 1};

    if (no < 0 || no >= m_meta.series_count)
    {
        qWarning() << "series" << no << "not in range of [ 0," << m_meta.series_count << "]";
        return;
    }
    if (m_meta.series == no) return;
    m_meta.series = no;
    auto pixels = m_doc->elementsByTagName("Image").at(no).firstChildElement("Pixels");
    int bpp = 1;
    m_meta.image_count = pixels.elementsByTagName("Plane").size();
    m_meta.size_x = pixels.attribute("SizeX").toInt();
    m_meta.size_y = pixels.attribute("SizeY").toInt();
    m_meta.size_z = pixels.attribute("SizeZ").toInt();
    m_meta.size_t = pixels.attribute("SizeT").toInt();
    m_meta.size_c = m_meta.image_count / m_meta.size_z / m_meta.size_t; // effective
    m_meta.physical_size_x =
        pixels.attribute("PhysicalSizeX").toDouble() * lengthUnitTransfer(pixels.attribute("PhysicalSizeXUnit"));
    m_meta.physical_size_y =
        pixels.attribute("PhysicalSizeY").toDouble() * lengthUnitTransfer(pixels.attribute("PhysicalSizeYUnit"));
    m_meta.physical_size_z =
        pixels.attribute("PhysicalSizeZ").toDouble() * lengthUnitTransfer(pixels.attribute("PhysicalSizeZUnit"));
    m_meta.physical_size_t = timeUnitTransfer(pixels.attribute("TimeIncrementUnit"));
    if (auto it = std::find(std::cbegin(typeStr), std::cend(typeStr), pixels.attribute("Type"));
        it == std::cend(typeStr))
        qCritical() << "unknown pixel type:" << pixels.attribute("Type");
    else
    {
        auto d = std::distance(std::cbegin(typeStr), it);
        m_meta.pixel_type = type[d];
        bpp = bytesPerPixel[d];
    }
    auto channels = pixels.elementsByTagName("Channel");
    m_meta.channel_colors.resize(channels.size());
    for (auto c = 0; c < channels.size(); c++)
    {
        auto color = channels.at(c).toElement().attribute("Color").toLong();
        m_meta.channel_colors[c] = {(color >> 24) & 0xff, (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff};
    }
    // https://github.com/ome/bioformats/blob/develop/components/formats-api/src/loci/formats/FormatReader.java#L760
    m_meta.rgb_channel_count = m_meta.size_c == 0 ? 0 : pixels.attribute("SizeC").toInt() / m_meta.size_c;
    m_meta.plane_size = m_meta.size_x * m_meta.size_y * m_meta.rgb_channel_count * bpp;
}

int SeriesReader::getImageCount()
{
    return m_meta.image_count;
}

int SeriesReader::getSeriesCount()
{
    return m_meta.series_count;
}

int SeriesReader::getSeries()
{
    return m_meta.series;
}

int SeriesReader::getSizeX()
{
    return m_meta.size_x;
}

int SeriesReader::getSizeY()
{
    return m_meta.size_y;
}

int SeriesReader::getSizeZ()
{
    return m_meta.size_z;
}

int SeriesReader::getSizeC()
{
    return m_meta.size_c;
}

int SeriesReader::getSizeT()
{
    return m_meta.size_t;
}

double SeriesReader::getPhysSizeX()
{
    return m_meta.physical_size_x;
}

double SeriesReader::getPhysSizeY()
{
    return m_meta.physical_size_y;
}

double SeriesReader::getPhysSizeZ()
{
    return m_meta.physical_size_z;
}

double SeriesReader::getPhysSizeT()
{
    return m_meta.physical_size_t;
}

int SeriesReader::getPixelType()
{
    return m_meta.pixel_type;
}

int SeriesReader::getRGBChannelCount()
{
    return m_meta.rgb_channel_count;
}

std::array<int, 4> SeriesReader::getChannelColor(int channel)
{
    return m_meta.channel_colors.at(channel);
}

int SeriesReader::getPlaneSize()
{
    return m_meta.plane_size;
}

int SeriesReader::getPlaneIndex(int z, int c, int t)
{
    auto planes =
        m_doc->elementsByTagName("Image").at(m_meta.series).firstChildElement("Pixels").elementsByTagName("Plane");
    for (auto p = 0; p < planes.size(); p++)
    {
        auto e = planes.at(p).toElement();
        auto cc = e.attribute("TheC").toInt();
        auto tt = e.attribute("TheT").toInt();
        auto zz = e.attribute("TheZ").toInt();
        if (cc == c && tt == t && zz == z) return p;
    }
    qCritical() << "can not getPlaneIndex for z =" << z << ", c =" << c << ", t =" << t;
    return -1;
}

std::array<int, 3> SeriesReader::getZCTCoords(int index)
{

    auto planes =
        m_doc->elementsByTagName("Image").at(m_meta.series).firstChildElement("Pixels").elementsByTagName("Plane");
    if (index < 0 || index >= planes.size())
    {
        qCritical() << "can not getZCTCoords for index =" << index;
        return {-1, -1, -1};
    }
    auto e = planes.at(index).toElement();
    auto c = e.attribute("TheC").toInt();
    auto t = e.attribute("TheT").toInt();
    auto z = e.attribute("TheZ").toInt();
    return {z, c, t};
}

cv::Mat SeriesReader::getPlane(int no)
{
    auto [z, c, t] = getZCTCoords(no);
    if (z * c * t < 0) return {};

    // `xxx_S%%sZ%%zC%%cT%%t.tiff`
    QRegularExpression re(".*_S(\\d+)Z(\\d+)C(\\d+)T(\\d+)");
    QDirIterator iter(QString::fromStdString(m_folder), QStringList() << "*.tiff", QDir::Files);
    while (iter.hasNext())
    {
        auto fn = iter.next();
        QRegularExpressionMatch match = re.match(fn);
        if (match.hasMatch())
        {
            int ss = match.captured(1).toInt();
            if (ss != m_meta.series) continue;

            int zz = match.captured(2).toInt();
            int cc = match.captured(3).toInt();
            int tt = match.captured(4).toInt();
            // FIXME: cv::imread image seems darker than bf
            if (zz == z && cc == c && tt == t) return cv::imread(fn.toStdString(), cv::IMREAD_GRAYSCALE);
        }
    }
    return {};
}

void SeriesReader::meta::PrintSelf() const
{
    qInfo() << "series_count" << series_count << "\nseries" << series << "\nimage_count" << image_count << "\nsize_x"
            << size_x << "\nsize_y" << size_y << "\nsize_z" << size_z << "\nsize_c" << size_c << "\nsize_t" << size_t
            << "\nphysical_size_x" << physical_size_x << "\nphysical_size_y" << physical_size_y << "\nphysical_size_z"
            << physical_size_z << "\nphysical_size_t" << physical_size_t << "\npixel_type" << pixel_type
            << "\nrgb_channel_count" << rgb_channel_count << "\nplane_size" << plane_size;
}
