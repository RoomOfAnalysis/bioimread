#pragma once

#include <string>
#include <array>
#include <memory>
#include <vector>

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
    bool reopen();

    void setSeries(int no);

    int getImageCount() const;
    int getSeriesCount() const;
    int getSeries() const;
    int getSizeX() const;
    int getSizeY() const;
    int getSizeZ() const;
    // effective
    int getSizeC() const;
    int getSizeT() const;
    // mm
    double getPhysSizeX() const;
    // mm
    double getPhysSizeY() const;
    // mm
    double getPhysSizeZ() const;
    // s
    double getPhysSizeT() const;
    PixelType getPixelType() const;
    int getBytesPerPixel() const;
    int getRGBChannelCount() const;
    std::array<int, 4> getChannelColor(int channel) const;
    int getPlaneSize() const;

    int getPlaneIndex(int z, int c, int t) const;
    std::array<int, 3> getZCTCoords(int index) const;
    std::unique_ptr<char[]> getPlane(int no) const;
    std::unique_ptr<std::vector<std::array<unsigned char, 3>>> get8BitLut() const;
    std::unique_ptr<std::vector<std::array<short, 3>>> get16BitLut() const;

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};