#pragma once

#include <string>
#include <opencv2/core.hpp>

class QDomDocument;

class SeriesReader
{
public:
    SeriesReader();
    ~SeriesReader();

    bool open(std::string metaXmlFilePath);
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

private:
    QDomDocument* m_doc = nullptr;
    struct meta
    {
        int series_count{};
        int series{-1};
        int image_count{};
        int size_x{};
        int size_y{};
        int size_z{};
        int size_c{};
        int size_t{};
        double physical_size_x{};
        double physical_size_y{};
        double physical_size_z{};
        double physical_size_t{};
        int pixel_type{};
        int rgb_channel_count{};
        std::vector<std::array<int, 4>> channel_colors{};
        int plane_size{};

        void PrintSelf() const;
    };
    meta m_meta{};
    std::string m_folder{};
};