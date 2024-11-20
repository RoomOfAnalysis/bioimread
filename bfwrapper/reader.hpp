#pragma once

#include <string>
#include <array>
#include <memory>

class Reader
{
public:
    enum class PixelType : int
    {
        INT8 = 0,
        UINT8,
        INT16,
        UINT16,
        INT32,
        UINT32,
        FLOAT,
        DOUBLE,
        BIT
    };

    static std::string pixelTypeStr(PixelType pixelType);
    static int getBytesPerPixel(PixelType pixelType);

public:
    Reader();
    ~Reader();

    bool open(std::string filePath);
    void close();

    void setSeries(int no);

    int getImageCount();
    int getSeriesCount();
    int getSeries();
    int getSizeX();
    int getSizeY();
    int getSizeZ();
    // effective
    int getSizeC();
    int getSizeT();
    // mm
    double getPhysSizeX();
    // mm
    double getPhysSizeY();
    // mm
    double getPhysSizeZ();
    // s
    double getPhysSizeT();
    PixelType getPixelType();
    int getBytesPerPixel();
    int getRGBChannelCount();
    std::array<int, 4> getChannelColor(int channel);
    int getPlaneSize();
    int getPlaneIndex(int z, int c, int t);
    std::array<int, 3> getZCTCoords(int index);
    std::unique_ptr<char[]> getPlane(int no);

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};