#pragma once

#include <memory>
#include <vector>
#include <string>
#include <utility>

class Reader;

// almost same as https://github.com/Harold2017/DeepZoomCpp
// but with different `get_tile` return type (PNG/JPG bytes instead of ARGB bytes)
class DeepZoomGenerator
{
public:
    enum class ImageFormat : int
    {
        PNG = 0,
        JPG
    };

    DeepZoomGenerator(std::string filepath, int tile_size = 254, int overlap = 1, ImageFormat format = ImageFormat::PNG,
                      float quality = 0.75f);
    ~DeepZoomGenerator();

    DeepZoomGenerator(DeepZoomGenerator const&) = delete;
    DeepZoomGenerator& operator=(DeepZoomGenerator const&) = delete;

    DeepZoomGenerator(DeepZoomGenerator&&) = default;
    DeepZoomGenerator& operator=(DeepZoomGenerator&&) = default;

    // deepzoom levels
    int level_count() const;
    // tile dimensions <col, row>
    std::vector<std::pair<int, int>> level_tiles() const;
    // deepzoom level dimensions <x, y>
    std::vector<std::pair<int, int>> level_dimensions() const;
    int tile_count() const;
    // PNG/JPG bytes
    std::vector<unsigned char> get_tile(int dz_level, int col, int row) const;
    // <<x, y>, slide_level, <width, height>>
    std::tuple<std::pair<int, int>, int, std::pair<int, int>> get_tile_coordinates(int dz_level, int col,
                                                                                   int row) const;
    // <width, height>
    std::pair<int, int> get_tile_dimensions(int dz_level, int col, int row) const;
    // XML
    std::string get_dzi() const;
    double get_mpp() const;

private:
    auto _get_tile_info(int dz_level, int col, int row) const
        -> std::pair<std::tuple<std::pair<int, int>, // l0_location
                                int,                 // slide_level
                                std::pair<int, int>  // l_size
                                >,
                     std::pair<int, int> // z_size
                     >;
    auto _get_best_level_for_downsample(double downsample) const -> int;

private:
    std::unique_ptr<Reader> m_reader = nullptr;
    int m_tile_size =
        512; // the width and height of a single tile, for best viewer performance, tile_size + 2 * overlap should be a power of two
    int m_overlap = 1; // the number of extra pixels to add to each interior edge of a tile
    ImageFormat m_format = ImageFormat::PNG;
    float m_quality = 0.75f;
    double m_mpp = 1e-6;
    int m_levels = 0;                                  // slide levels
    int m_dz_levels = 0;                               // deepzoom levels
    std::vector<std::pair<int, int>> m_l_dimensions;   // slide level dimensions
    std::vector<std::pair<int, int>> m_dzl_dimensions; // deepzoom level dimensions
    std::vector<std::pair<int, int>> m_t_dimensions;   // tile dimensions
    std::vector<int> m_preferred_slide_levels;         // preferred slide levels for each deepzoom level
    std::vector<double> m_level_downsamples;           // slide level downsample factors
    std::vector<double>
        m_level_dz_downsamples; // deepzoom level downsample factors (ratio of deepzoom level to slide level)
    std::vector<double> m_level_0_dz_downsamples; // total downsamples for each Deep Zoom level (2 ** x)
};
