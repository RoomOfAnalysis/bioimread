#include "deepzoom.hpp"

#include <iostream>

std::string Base64_Encode(unsigned char const* src, size_t len);

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << ": <slide path>" << std::endl;
        return -1;
    }

    std::string url = argv[1];
    DeepZoomGenerator slide_handler(url, 254, 1);
    std::cout << slide_handler.get_dzi() << std::endl;

    std::cout << "MPP: " << slide_handler.get_mpp() << std::endl;

    auto png_bytes = slide_handler.get_tile(slide_handler.level_count() / 2, 0, 0);
    std::cout << "length: " << png_bytes.size() << std::endl;
    auto str = "data:image/png;base64," + Base64_Encode(png_bytes.data(), png_bytes.size());
    std::cout << str << std::endl;

    return 0;
}

/*
* Base64 encoding/decoding (RFC1341)
* Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
*/
std::string Base64_Encode(unsigned char const* src, size_t len)
{
    static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    unsigned char *out, *pos;
    unsigned char const *end, *in;

    size_t olen;

    olen = 4 * ((len + 2) / 3);

    if (olen < len) return std::string();

    std::string outStr;
    outStr.resize(olen);
    out = (unsigned char*)&outStr[0];

    end = src + len;
    in = src;
    pos = out;
    while (end - in >= 3)
    {
        *pos++ = base64_table[in[0] >> 2];
        *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
    }

    if (end - in)
    {
        *pos++ = base64_table[in[0] >> 2];
        if (end - in == 1)
        {
            *pos++ = base64_table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        }
        else
        {
            *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
    }

    return outStr;
}
