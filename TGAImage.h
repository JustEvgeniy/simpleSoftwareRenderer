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
    union {
        struct {
            uint8_t b, g, r, a;
        };
        uint8_t raw[4];
        uint32_t val;
    };
    uint32_t bytesPerPixel;

    TGAColor() : val(0), bytesPerPixel(1) {
    }

    TGAColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255) : b(B), g(G), r(R), a(A),
                                                                 bytesPerPixel(4) {
    }

    TGAColor(int32_t v, uint32_t bpp) : val(static_cast<uint32_t>(v)), bytesPerPixel(bpp) {
    }

    TGAColor(const TGAColor &c) : val(c.val), bytesPerPixel(c.bytesPerPixel) {
    }

    TGAColor(const uint8_t p[], uint32_t bpp) : val(0), bytesPerPixel(bpp) {
        for (int i = 0; i < bpp; i++) {
            raw[i] = p[i];
        }
    }

    TGAColor &operator=(const TGAColor &c) {
        if (this != &c) {
            bytesPerPixel = c.bytesPerPixel;
            val = c.val;
        }
        return *this;
    }

    TGAColor intensity(float d) {
        TGAColor newColor(val, bytesPerPixel);
        for (uint8_t &i : newColor.raw) {
            i = static_cast<uint8_t>(i * d);
        }
        return newColor;
    }
};


class TGAImage {
protected:
    uint8_t *data;
    uint32_t width;
    uint32_t height;
    uint32_t bytesPerPixel;

    bool load_rle_data(std::ifstream &in);

    bool unload_rle_data(std::ofstream &out);

public:
    enum Format {
        GRAYSCALE = 1, RGB = 3, RGBA = 4
    };

    TGAImage();

    TGAImage(uint32_t w, uint32_t h, uint32_t bpp);

    TGAImage(const TGAImage &img);

    ~TGAImage();

    TGAImage &operator=(const TGAImage &img);

    bool read_tga_file(std::string filename);

    bool write_tga_file(std::string filename, bool rle = true);

    bool flip_horizontally();

    bool flip_vertically();

    bool scale(const uint32_t &w, const uint32_t &h);

    TGAColor get(const int32_t &x, const int32_t &y) const;

    bool set(const int32_t &x, const int32_t &y, const TGAColor &c);

    uint32_t get_width();

    uint32_t get_height();

    uint32_t get_bytesPerPixel();

    uint8_t *buffer();

    void clear();
};

#endif //SIMPLESOFTWARERENDERER_TGAIMAGE_H
