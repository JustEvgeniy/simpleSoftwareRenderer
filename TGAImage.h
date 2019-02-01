//
// Created by ju5t on 29.01.19.
//

#ifndef SIMPLESOFTWARERENDERER_TGAIMAGE_H
#define SIMPLESOFTWARERENDERER_TGAIMAGE_H

#include <fstream>

#pragma pack(push, 1)
struct TGA_Header {
    char idLength;
    char colorMapType;
    char dataTypeCode;
    short colorMapOrigin;
    short colorMapLength;
    char colorMapDepth;
    short x_origin;
    short y_origin;
    short width;
    short height;
    char bitsPerPixel;
    char imageDescriptor;
};
#pragma pack(pop)


struct TGAColor {
    uint8_t bgra[4];
    uint8_t bytesPerPixel;

    TGAColor() : bgra(), bytesPerPixel(1) {
        for (int i = 0; i < 4; ++i) {
            bgra[i] = 0;
        }
    }

    TGAColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255) : bgra(), bytesPerPixel(4) {
        bgra[0] = B;
        bgra[1] = G;
        bgra[2] = R;
        bgra[3] = A;
    }

    TGAColor(uint8_t v, uint8_t bpp) : bgra(), bytesPerPixel(bpp) {
        for (int i = 0; i < 4; ++i) {
            bgra[i] = 0;
        }
        bgra[0] = v;
    }

    TGAColor(const uint8_t p[], uint8_t bpp) : bgra(), bytesPerPixel(bpp) {
        for (int i = 0; i < bpp; ++i) {
            bgra[i] = p[i];
        }
        for (int i = bpp; i < 4; ++i) {
            bgra[i] = 0;
        }
    }

    TGAColor operator*(float intensity) const {
        TGAColor res = *this;

        for (int i = 0; i < 4; ++i) {
            res.bgra[i] = static_cast<uint8_t>(bgra[i] * intensity);
        }

        return res;
    }
};

class TGAImage {
protected:
    uint8_t *data;
    int32_t width, height;
    uint8_t bytesPerPixel;

    bool load_rle_data(std::ifstream &in);

    bool unload_rle_data(std::ofstream &out);

public:
    enum Format {
        GRAYSCALE = 1, RGB = 3, RGBA = 4
    };

    TGAImage();

    TGAImage(int32_t w, int32_t h, uint8_t bpp);

    TGAImage(const TGAImage &img);

    ~TGAImage();

    TGAImage &operator=(const TGAImage &img);

    bool read_tga_file(std::string filename);

    bool write_tga_file(std::string filename, bool rle = true);

    bool flip_horizontally();

    bool flip_vertically();

    bool scale(const int32_t &w, const int32_t &h);

    TGAColor get(const int32_t &x, const int32_t &y);

    bool set(const int32_t &x, const int32_t &y, const TGAColor &c);

    int32_t get_width();

    int32_t get_height();

    uint8_t get_bytesPerPixel();

    uint8_t *buffer();

    void clear();
};

#endif //SIMPLESOFTWARERENDERER_TGAIMAGE_H
