#pragma once

#include <QImage>

#include "../bfwrapper/reader.hpp"

QImage readPlaneToQimage(Reader const& reader, int plane_index);
