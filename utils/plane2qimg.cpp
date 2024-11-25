#include "plane2qimg.hpp"

#include <qDebug>

template <typename T> std::pair<T, T> minMax(char* bytes, qsizetype length)
{
    T min = std::numeric_limits<T>::max(), max = std::numeric_limits<T>::min();
    auto* ptr = reinterpret_cast<T*>(bytes);
    for (qsizetype i = 0; i < length; i++)
    {
        // non-saturated black
        if (ptr[i] > std::numeric_limits<T>::min()) min = std::min(ptr[i], min);
        // non-saturated white
        if (ptr[i] < std::numeric_limits<T>::max()) max = std::max(ptr[i], max);
    }
    return std::make_pair(min, max);
}

template <typename T> bool m_qFuzzyCompare(T a, T b)
{
    if constexpr (std::is_floating_point_v<T>)
        return qFuzzyCompare(a, b);
    else
        return a == b;
}

template <typename T> QList<uint> prepareLut(std::vector<std::array<T, 3>>* lut)
{
    if (lut == nullptr || lut->empty()) return {};

    QList<uint> qlut(lut->size());
    for (auto i = 0; i < lut->size(); i++)
        qlut[i] = qRgb((*lut)[i][0], (*lut)[i][1], (*lut)[i][2]);
    return qlut;
}

template <typename T> QImage toIndexed8(char* bytes, int width, int height, QList<uint> const& lut = {})
{
    auto length = static_cast<qsizetype>(width) * height;
    auto [min, max] = minMax<T>(bytes, length);
    if (m_qFuzzyCompare(min, max))
    {
        qCritical() << "invalid min max range";
        return QImage();
    }
    //qDebug() << "minMax: " << min << max;
    auto scale = 255. / (max - min);
    QImage img(width, height, QImage::Format::Format_Indexed8);
    if (lut.isEmpty())
    {
        QList<uint> default_lut(256);
        // default lut is simple grayscale
        // also it can be used to set false color, e.g. `QColor::fromHslF(i / 255.f * 0.8f, 0.95f, 0.5f).rgba()`
        for (auto i = 0; i < 256; i++)
            default_lut[i] = qRgb(i, i, i);
        img.setColorTable(default_lut);
    }
    else
        img.setColorTable(lut);
    auto* in_p = reinterpret_cast<T*>(bytes);
    auto* out_p = reinterpret_cast<uint8_t*>(img.bits());
    for (auto i = 0; i < length; i++)
        out_p[i] = std::clamp(static_cast<int>(std::round((in_p[i] - min) * scale)), 0, 255);
    return img;
}

QImage readPlaneToQimage(Reader const& reader, int plane_index)
{
    auto width = reader.getSizeX();
    auto height = reader.getSizeY();
    auto bytesPerPixel = reader.getBytesPerPixel();
    auto type = reader.getPixelType();
    auto rgbChannelCount = reader.getRGBChannelCount();
    auto plane = reader.getPlane(plane_index);
    auto bytes = plane.get();
    auto lut8 = reader.get8BitLut();

    //auto lut16 = reader.get16BitLut();
    //QList<uint> lut;
    //if (lut8)
    //    lut = prepareLut(lut8.get());
    //else if (lut16)
    //    lut = prepareLut(lut16.get());

    // FIXME: 16bit color table may have 65536 colors... which means too much for `uint8`
    // only indexed8 qimage support color table...
    // so may need to convert 16bit image to RGB32 (like what i do in gray16 false color...)
    // here just ignore 16bit lut...
    auto lut = prepareLut(lut8.get());

    //qDebug() << rgbChannelCount << Reader::pixelTypeStr(type) << bytesPerPixel << width << height;

    if (rgbChannelCount == 1)
    {
        switch (type)
        {
        // 8bit lut
        case Reader::PixelType::UINT8:
            return toIndexed8<uint8_t>(bytes, width, height, lut);
        case Reader::PixelType::INT8:
            return toIndexed8<int8_t>(bytes, width, height, lut);
        // 16bit lut
        case Reader::PixelType::UINT16:
            return toIndexed8<uint16_t>(bytes, width, height, lut);
        case Reader::PixelType::INT16:
            return toIndexed8<int16_t>(bytes, width, height, lut);
        // followings do NOT have LUT
        case Reader::PixelType::BIT:
            return QImage((uchar*)bytes, width, height, static_cast<qsizetype>(width) * bytesPerPixel,
                          QImage::Format::Format_Mono)
                .copy();
        case Reader::PixelType::INT32:
            return toIndexed8<int32_t>(bytes, width, height);
        case Reader::PixelType::UINT32:
            return toIndexed8<uint32_t>(bytes, width, height);
        case Reader::PixelType::FLOAT:
            return toIndexed8<float>(bytes, width, height);
        case Reader::PixelType::DOUBLE:
            return toIndexed8<double>(bytes, width, height);
        default:
            return QImage();
        }
    }
    else if (rgbChannelCount == 3)
    {
        if (type == Reader::PixelType::UINT8)
            // FIXME: bioformats read jpg into 3 * 3 subblocks...
            return QImage((uchar*)bytes, width, height, static_cast<qsizetype>(width) * bytesPerPixel * 3,
                          QImage::Format::Format_RGB888)
                .copy();
        else
        {
            qCritical() << "readPlaneToQimage: unsupported pixel type:" << Reader::pixelTypeStr(type)
                        << "for 3 channels";
            return QImage();
        }
    }
    else if (rgbChannelCount == 4)
    {
        // TODO: convert to premultuplied format to increase painting performance?
        // https://stackoverflow.com/questions/54564508/why-does-not-using-premultiplied-alpha-have-significantly-worse-performance
        switch (type)
        {
        case Reader::PixelType::UINT8:
            return QImage((uchar*)bytes, width, height, static_cast<qsizetype>(width) * bytesPerPixel * 4,
                          QImage::Format::Format_RGBA8888)
                .copy();
        case Reader::PixelType::UINT16:
            return QImage((uchar*)bytes, width, height, static_cast<qsizetype>(width) * bytesPerPixel * 4,
                          QImage::Format::Format_RGBA64)
                .copy();
        case Reader::PixelType::FLOAT:
            return QImage((uchar*)bytes, width, height, static_cast<qsizetype>(width) * bytesPerPixel * 4,
                          QImage::Format::Format_RGBA32FPx4)
                .copy();
        default:
            qCritical() << "readPlaneToQimage: unsupported pixel type:" << Reader::pixelTypeStr(type)
                        << "for 4 channels";
            return QImage();
        }
    }
    else
    {
        qCritical() << "readPlaneToQimage: unsupported rgb channels count:" << rgbChannelCount;
        return QImage();
    }
}
