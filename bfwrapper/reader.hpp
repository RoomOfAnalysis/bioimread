#pragma once

#include <string>
#include <memory>
#include <opencv2/core.hpp>

class Reader
{
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
    int getPixelType();
    int getRGBChannelCount();
    std::array<int, 4> getChannelColor(int channel);
    int getPlaneSize();
    int getPlaneIndex(int z, int c, int t);
    std::array<int, 3> getZCTCoords(int index);
    cv::Mat getPlane(int no);

    std::string pixelTypeStr(int pixelType);
    int pixelType2CVType(int pixelType);

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};