#pragma once

#include <QImage>

std::pair<uint16_t, uint16_t> minMax(QImage const& img, bool non_saturated = true);

QImage gray16ToGray8(QImage const& img, uint16_t min = 0, uint16_t max = 0);

/*
* black body style
* output image is RGB32
* default mapping is as follows
* 0      - 0x5555 -> black to red
* 0x5555 - 0xAAAA -> red to white
* 0xAAAA - 0xFFFF -> white to blue
* if useMinMax == true
* d = max - min
* 0         - d / 3     -> black to red
* d / 3     - 2 * d / 3 -> red to white
* 2 * d / 3 - d         -> white to blue
*/
QImage falseColor(QImage const& img, bool useMinMax = true);
