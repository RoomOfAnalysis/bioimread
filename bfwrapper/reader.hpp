#pragma once

#include <string>
#include <array>
#include <memory>
#include <vector>
#include <optional>

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

    void setFlattenedResolutions(bool flag);
    bool open(std::string filePath);
    void close();
    bool reopen();

    void setSeries(int no);

    std::string getMetaXML() const;

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
    int getBitsPerPixel() const;
    int getBytesPerPixel() const;
    int getRGBChannelCount() const;
    std::optional<std::array<int, 4>> getChannelColor(int channel) const;
    int getPlaneSize() const;

    int getPlaneIndex(int z, int c, int t) const;
    std::array<int, 3> getZCTCoords(int index) const;
    std::unique_ptr<char[]> getPlane(int no) const;
    std::unique_ptr<std::vector<std::array<unsigned char, 3>>> get8BitLut() const;
    std::unique_ptr<std::vector<std::array<short, 3>>> get16BitLut() const;

    int getOptimalTileWidth() const;
    int getOptimalTileHeight() const;
    std::unique_ptr<char[]> getTile(int no, int x, int y, int w, int h) const;

    // only available after `setFlattenedResolutions`
    int getResolutionCount() const;
    // only available after `setFlattenedResolutions`
    void setResolution(int level);

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};