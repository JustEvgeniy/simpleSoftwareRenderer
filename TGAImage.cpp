//
// Created by ju5t on 29.01.19.
//

#include <cstring>
#include <iostream>
#include "TGAImage.h"

bool TGAImage::load_rle_data(std::ifstream &in) {
    uint64_t pixelCount = width * height;
    uint64_t currentPixel = 0;
    uint64_t currentByte = 0;
    TGAColor colorBuffer;
    do {
        int32_t chunkHeader = 0;
        chunkHeader = in.get();
        if (!in.good()) {
            std::cerr << "An error occurred while reading the data\n";
            return false;
        }

        if (chunkHeader < 128) {
            chunkHeader++;
            for (int i = 0; i < chunkHeader; i++) {
                in.read(reinterpret_cast<char *>(colorBuffer.bgra), bytesPerPixel);
                if (!in.good()) {
                    std::cerr << "An error occurred while reading the header\n";
                    return false;
                }

                for (int t = 0; t < bytesPerPixel; t++)
                    data[currentByte++] = colorBuffer.bgra[t];

                currentPixel++;

                if (currentPixel > pixelCount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        } else {
            chunkHeader -= 127;
            in.read(reinterpret_cast<char *>(colorBuffer.bgra), bytesPerPixel);
            if (!in.good()) {
                std::cerr << "An error occurred while reading the header\n";
                return false;
            }

            for (int i = 0; i < chunkHeader; i++) {
                for (int t = 0; t < bytesPerPixel; t++)
                    data[currentByte++] = colorBuffer.bgra[t];

                currentPixel++;

                if (currentPixel > pixelCount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        }
    } while (currentPixel < pixelCount);
    return true;
}

//TODO: it is not necessary to break a raw chunk for two equal pixels (for the matter of the resulting size)
bool TGAImage::unload_rle_data(std::ofstream &out) {
    const uint8_t max_chunk_length = 128;
    uint64_t nPixels = width * height;
    uint64_t curPixel = 0;

    while (curPixel < nPixels) {
        uint64_t chunkStart = curPixel * bytesPerPixel;
        uint64_t curByte = curPixel * bytesPerPixel;
        uint8_t run_length = 1;
        bool raw = true;
        while (curPixel + run_length < nPixels && run_length < max_chunk_length) {
            bool succ_eq = true;
            for (int t = 0; succ_eq && t < bytesPerPixel; t++) {
                succ_eq = (data[curByte + t] == data[curByte + t + bytesPerPixel]);
            }
            curByte += bytesPerPixel;
            if (run_length == 1) {
                raw = !succ_eq;
            }
            if (raw && succ_eq) {
                run_length--;
                break;
            }
            if (!raw && !succ_eq) {
                break;
            }
            run_length++;
        }

        curPixel += run_length;
        out.put(static_cast<char>(raw ? run_length - 1 : run_length + 127));
        if (!out.good()) {
            std::cerr << "Can't dump the tga file\n";
            return false;
        }

        out.write(reinterpret_cast<const char *>(data + chunkStart),
                  (raw ? run_length * bytesPerPixel : bytesPerPixel));
        if (!out.good()) {
            std::cerr << "Can't dump the tga file\n";
            return false;
        }
    }
    return true;
}

TGAImage::TGAImage() : data(nullptr), width(0), height(0), bytesPerPixel(0) {}

TGAImage::TGAImage(int32_t w, int32_t h, uint8_t bpp) : data(nullptr), width(w), height(h), bytesPerPixel(bpp) {
    uint64_t nBytes = width * height * bytesPerPixel;
    data = new uint8_t[nBytes];
    memset(data, 0, nBytes);
}

TGAImage::TGAImage(const TGAImage &img) : data(nullptr), width(img.width), height(img.height),
                                          bytesPerPixel(img.bytesPerPixel) {
    uint64_t nBytes = width * height * bytesPerPixel;
    data = new uint8_t[nBytes];
    memcpy(data, img.data, nBytes);
}

TGAImage::~TGAImage() {
    delete[] data;
}

TGAImage &TGAImage::operator=(const TGAImage &img) {
    if (this != &img) {
        delete[] data;
        width = img.width;
        height = img.height;
        bytesPerPixel = img.bytesPerPixel;

        uint64_t nBytes = width * height * bytesPerPixel;
        data = new uint8_t[nBytes];
        memcpy(data, img.data, nBytes);
    }
    return *this;
}

bool TGAImage::read_tga_file(std::string filename) {
    delete[] data;
    data = nullptr;

    std::ifstream in;
    in.open(filename, std::ios::binary);
    if (!in.is_open()) {
        in.close();
        std::cerr << "Can't open file " << filename << "\n";
        return false;
    }

    TGA_Header header{};
    in.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (!in.good()) {
        in.close();
        std::cerr << "An error occurred while reading the header\n";
        return false;
    }
    width = static_cast<uint32_t>(header.width);
    height = static_cast<uint32_t>(header.height);
    bytesPerPixel = static_cast<uint8_t>(header.bitsPerPixel >> 3);

    if (width <= 0 || height <= 0 || (bytesPerPixel != GRAYSCALE && bytesPerPixel != RGB && bytesPerPixel != RGBA)) {
        in.close();
        std::cerr << "Bad bpp (or width/height) value\n";
        return false;
    }

    uint64_t nBytes = bytesPerPixel * width * height;
    data = new uint8_t[nBytes];
    if (header.dataTypeCode == 2 || header.dataTypeCode == 3) {
        in.read((char *) data, nBytes);
        if (!in.good()) {
            in.close();
            std::cerr << "An error occurred while reading the data\n";
            return false;
        }
    } else if (header.dataTypeCode == 10 || header.dataTypeCode == 11) {
        if (!load_rle_data(in)) {
            in.close();
            std::cerr << "An error occurred while reading the data\n";
            return false;
        }
    } else {
        in.close();
        std::cerr << "Unknown file format " << (int) header.dataTypeCode << "\n";
        return false;
    }

    if (!(header.imageDescriptor & 0x20)) {
        flip_vertically();
    }

    if (header.imageDescriptor & 0x10) {
        flip_horizontally();
    }

    in.close();
    std::cerr << width << "x" << height << "/" << bytesPerPixel * 8 << "\n";
    return true;
}

bool TGAImage::write_tga_file(std::string filename, bool rle) {
    uint8_t developer_area_ref[4] = {0, 0, 0, 0};
    uint8_t extension_area_ref[4] = {0, 0, 0, 0};
    uint8_t footer[18] = {'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N', '-', 'X', 'F', 'I', 'L', 'E', '.', '\0'};

    std::ofstream out;
    out.open(filename, std::ios::binary);
    if (!out.is_open()) {
        out.close();
        std::cerr << "Can't open file " << filename << "\n";
        return false;
    }

    TGA_Header header{};
    memset(&header, 0, sizeof(header));
    header.bitsPerPixel = static_cast<char>(bytesPerPixel << 3);
    header.width = static_cast<short>(width);
    header.height = static_cast<short>(height);
    header.dataTypeCode = static_cast<char>(bytesPerPixel == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
    header.imageDescriptor = 0x20; // top-left origin

    out.write(reinterpret_cast<const char *>(&header), sizeof(header));
    if (!out.good()) {
        out.close();
        std::cerr << "Can't dump the tga file\n";
        return false;
    }

    if (!rle) {
        out.write(reinterpret_cast<const char *>(data), width * height * bytesPerPixel);
        if (!out.good()) {
            out.close();
            std::cerr << "Can't unload raw data\n";
            return false;
        }
    } else {
        if (!unload_rle_data(out)) {
            out.close();
            std::cerr << "Can't unload rle data\n";
            return false;
        }
    }

    out.write(reinterpret_cast<const char *>(developer_area_ref), sizeof(developer_area_ref));
    if (!out.good()) {
        out.close();
        std::cerr << "Can't dump the tga file\n";
        return false;
    }

    out.write(reinterpret_cast<const char *>(extension_area_ref), sizeof(extension_area_ref));
    if (!out.good()) {
        out.close();
        std::cerr << "Can't dump the tga file\n";
        return false;
    }

    out.write(reinterpret_cast<const char *>(footer), sizeof(footer));
    if (!out.good()) {
        out.close();
        std::cerr << "Can't dump the tga file\n";
        return false;
    }

    out.close();
    return true;
}

bool TGAImage::flip_horizontally() {
    if (!data)
        return false;

    int half = width >> 1;

    for (int i = 0; i < half; i++) {
        for (int j = 0; j < height; j++) {
            TGAColor c1 = get(i, j);
            TGAColor c2 = get(width - 1 - i, j);
            set(i, j, c2);
            set(width - 1 - i, j, c1);
        }
    }
    return true;
}

bool TGAImage::flip_vertically() {
    if (!data)
        return false;

    uint64_t bytes_per_line = width * bytesPerPixel;
    auto *line = new uint8_t[bytes_per_line];
    int half = height >> 1;

    for (int j = 0; j < half; j++) {
        uint64_t l1 = j * bytes_per_line;
        uint64_t l2 = (height - 1 - j) * bytes_per_line;
        memmove(line, (data + l1), bytes_per_line);
        memmove((data + l1), (data + l2), bytes_per_line);
        memmove((data + l2), line, bytes_per_line);
    }
    delete[] line;
    return true;
}

bool TGAImage::scale(const int32_t &w, const int32_t &h) {
    if (w <= 0 || h <= 0 || !data)
        return false;

    auto *tData = new uint8_t[w * h * bytesPerPixel];
    int nscanline = 0;
    int oscanline = 0;
    int erry = 0;

    uint64_t nlinebytes = w * bytesPerPixel;
    uint64_t olinebytes = width * bytesPerPixel;
    for (int j = 0; j < height; j++) {
        int errx = width - w;
        int nx = -bytesPerPixel;
        int ox = -bytesPerPixel;

        for (int i = 0; i < width; i++) {
            ox += bytesPerPixel;
            errx += w;

            while (errx >= width) {
                errx -= width;
                nx += bytesPerPixel;
                memcpy(tData + nscanline + nx, data + oscanline + ox, bytesPerPixel);
            }
        }
        erry += h;
        oscanline += olinebytes;

        while (erry >= height) {
            if (erry >= height << 1) // it means we jump over a scanline
                memcpy(tData + nscanline + nlinebytes, tData + nscanline, nlinebytes);
            erry -= height;
            nscanline += nlinebytes;
        }
    }

    delete[] data;
    data = tData;
    width = w;
    height = h;
    return true;
}

TGAColor TGAImage::get(const int32_t &x, const int32_t &y) {
    if (!data || x < 0 || y < 0 || x >= width || y >= height) {
        return {};
    }
    return {data + (x + y * width) * bytesPerPixel, bytesPerPixel};
}

bool TGAImage::set(const int32_t &x, const int32_t &y, const TGAColor &c) {
    if (!data || x < 0 || y < 0 || x >= width || y >= height) {
        return false;
    }
    memcpy(data + (x + y * width) * bytesPerPixel, c.bgra, bytesPerPixel);
    return true;
}

int32_t TGAImage::get_width() {
    return width;
}

int32_t TGAImage::get_height() {
    return height;
}

uint8_t TGAImage::get_bytesPerPixel() {
    return bytesPerPixel;
}

uint8_t *TGAImage::buffer() {
    return data;
}

void TGAImage::clear() {
    memset(data, 0, width * height * bytesPerPixel);
}
