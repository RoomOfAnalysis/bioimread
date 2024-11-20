#include "gray16.hpp"

#include <qDebug>

std::pair<uint16_t, uint16_t> minMax(QImage const& img, bool non_saturated)
{
    uint16_t min{0xFFFF}, max{0};
    auto* ptr = reinterpret_cast<const uint16_t*>(img.constBits());
    if (non_saturated)
    {
        for (auto i = 0; i < img.width() * img.height(); i++)
        {
            // non-saturated black
            if (ptr[i] > 0) min = std::min(ptr[i], min);
            // non-saturated white
            if (ptr[i] < 0xFFFF) max = std::max(ptr[i], max);
        }
        return std::make_pair(min, max);
    }
    else
    {
        for (auto i = 0; i < img.width() * img.height(); i++)
        {
            min = std::min(ptr[i], min);
            max = std::max(ptr[i], max);
        }
        return std::make_pair(min, max);
    }
}

QImage gray16ToGray8(QImage const& img, uint16_t min, uint16_t max)
{
    if (min == max || max == 0)
    {
        auto [min_, max_] = minMax(img);
        min = min_;
        max = max_;
    }
    if (max < min + 1)
    {
        qCritical() << "invalid min max range";
        return img;
    }
    auto scale = 255. / (max - min);
    QImage out(img.width(), img.height(), QImage::Format_Grayscale8);
    auto* out_p = reinterpret_cast<uint8_t*>(out.bits());
    auto* in_p = reinterpret_cast<const uint16_t*>(img.constBits());
    for (auto i = 0; i < img.width() * img.height(); i++)
        out_p[i] = std::clamp(static_cast<int>(std::round((in_p[i] - min) * scale)), 0, 255);
    return out;
}

QImage falseColor(QImage const& img, bool useMinMax)
{
    QImage out(img.width(), img.height(), QImage::Format_RGB32);
    auto* out_p = reinterpret_cast<uint32_t*>(out.bits());
    auto* in_p = reinterpret_cast<const uint16_t*>(img.constBits());
    uint16_t d = 0x5555;
    if (useMinMax)
    {
        auto [min, max] = minMax(img);
        d = (max - min) / 3.;
    }
    auto const scale = 255 / d;
    for (int i = 0; i < img.width() * img.height(); ++i)
    {
        if (in_p[i] < d)
        {
            // Black to Red
            auto tmp = static_cast<uint32_t>(std::round(in_p[i] * scale));
            out_p[i] = 0xFF000000 | (tmp << 16);
        }
        else if (in_p[i] >= d && in_p[i] < 2 * d)
        {
            // Red to White
            auto tmp = static_cast<uint32_t>(std::round((in_p[i] - d) * scale));
            out_p[i] = 0xFFFF0000 | (tmp << 8) | tmp;
        }
        else if (in_p[i] >= 2 * d)
        {
            // White to Blue
            auto tmp = 0xFF - static_cast<uint32_t>(std::round((in_p[i] - 2 * d) * scale));
            out_p[i] = 0xFF0000FF | (tmp << 16) | (tmp << 8);
        }
    }

    return out;
}
