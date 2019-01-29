//
// Created by ju5t on 29.01.19.
//

#include <cstring>
#include <iostream>
#include "TGAImage.h"

TGAImage::TGAImage() : data(nullptr), width(0), height(0), bytespp(0) {}

TGAImage::TGAImage(unsigned int w, unsigned int h, unsigned int bpp) : data(nullptr), width(w), height(h),
                                                                       bytespp(bpp) {
    unsigned long nBytes = width * height * bytespp;
    data = new unsigned char[nBytes];
    memset(data, 0, nBytes);
}

TGAImage::TGAImage(const TGAImage &img) {
    width = img.width;
    height = img.height;
    bytespp = img.bytespp;

    unsigned long nBytes = width * height * bytespp;
    data = new unsigned char[nBytes];
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
        bytespp = img.bytespp;

        unsigned long nBytes = width * height * bytespp;
        data = new unsigned char[nBytes];
        memcpy(data, img.data, nBytes);
    }
    return *this;
}

bool TGAImage::read_tga_file(const char *filename) {
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
    in.read((char *) &header, sizeof(header));
    if (!in.good()) {
        in.close();
        std::cerr << "An error occurred while reading the header\n";
        return false;
    }
    width = header.width;
    height = header.height;
    bytespp = header.bitsperpixel >> 3;

    if (width <= 0 || height <= 0 || (bytespp != GRAYSCALE && bytespp != RGB && bytespp != RGBA)) {
        in.close();
        std::cerr << "Bad bpp (or width/height) value\n";
        return false;
    }

    unsigned long nBytes = bytespp * width * height;
    data = new unsigned char[nBytes];
    if (header.datatypecode == 2 || header.datatypecode == 3) {
        in.read((char *) data, nBytes);
        if (!in.good()) {
            in.close();
            std::cerr << "An error occurred while reading the data\n";
            return false;
        }
    } else if (header.datatypecode == 10 || header.datatypecode == 11) {
        if (!load_rle_data(in)) {
            in.close();
            std::cerr << "An error occurred while reading the data\n";
            return false;
        }
    } else {
        in.close();
        std::cerr << "Unknown file format " << (int) header.datatypecode << "\n";
        return false;
    }

    if (!(header.imagedescriptor & 0x20)) {
        flip_vertically();
    }

    if (header.imagedescriptor & 0x10) {
        flip_horizontally();
    }

    in.close();
    std::cerr << width << "x" << height << "/" << bytespp * 8 << "\n";
    return true;
}

bool TGAImage::load_rle_data(std::ifstream &in) {
    unsigned long pixelCount = width * height;
    unsigned long currentPixel = 0;
    unsigned long currentByte = 0;
    TGAColor colorBuffer;
    do {
        unsigned char chunkHeader = 0;
        chunkHeader = in.get();
        if (!in.good()) {
            std::cerr << "An error occurred while reading the data\n";
            return false;
        }

        if (chunkHeader < 128) {
            chunkHeader++;
            for (int i = 0; i < chunkHeader; i++) {
                in.read((char *) colorBuffer.raw, bytespp);
                if (!in.good()) {
                    std::cerr << "An error occurred while reading the header\n";
                    return false;
                }

                for (int t = 0; t < bytespp; t++)
                    data[currentByte++] = colorBuffer.raw[t];

                currentPixel++;

                if (currentPixel > pixelCount) {
                    std::cerr << "Too many pixels read\n";
                    return false;
                }
            }
        } else {
            chunkHeader -= 127;
            in.read((char *) colorBuffer.raw, bytespp);
            if (!in.good()) {
                std::cerr << "An error occurred while reading the header\n";
                return false;
            }

            for (int i = 0; i < chunkHeader; i++) {
                for (int t = 0; t < bytespp; t++)
                    data[currentByte++] = colorBuffer.raw[t];

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

bool TGAImage::write_tga_file(const char *filename, bool rle) {
    unsigned char developer_area_ref[4] = {0, 0, 0, 0};
    unsigned char extension_area_ref[4] = {0, 0, 0, 0};
    unsigned char footer[18] = {'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N', '-', 'X', 'F', 'I', 'L', 'E', '.',
                                '\0'};

    std::ofstream out;
    out.open(filename, std::ios::binary);
    if (!out.is_open()) {
        out.close();
        std::cerr << "Can't open file " << filename << "\n";
        return false;
    }

    TGA_Header header{};
    memset((void *) &header, 0, sizeof(header));
    header.bitsperpixel = bytespp << 3;
    header.width = width;
    header.height = height;
    header.datatypecode = (bytespp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));

    header.imagedescriptor = 0x20; // top-left origin
    out.write((char *) &header, sizeof(header));
    if (!out.good()) {
        out.close();
        std::cerr << "Can't dump the tga file\n";
        return false;
    }

    if (!rle) {
        out.write((char *) data, width * height * bytespp);
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

    out.write((char *) developer_area_ref, sizeof(developer_area_ref));
    if (!out.good()) {
        out.close();
        std::cerr << "Can't dump the tga file\n";
        return false;
    }

    out.write((char *) extension_area_ref, sizeof(extension_area_ref));
    if (!out.good()) {
        out.close();
        std::cerr << "can't dump the tga file\n";
        return false;
    }

    out.write((char *) footer, sizeof(footer));
    if (!out.good()) {
        out.close();
        std::cerr << "Can't dump the tga file\n";
        return false;
    }

    out.close();
    return true;
}

// TODO: it is not necessary to break a raw chunk for two equal pixels (for the matter of the resulting size)
bool TGAImage::unload_rle_data(std::ofstream &out) {
    const unsigned char max_chunk_length = 128;
    unsigned long nPixels = width * height;
    unsigned long curPixel = 0;
    while (curPixel < nPixels) {
        unsigned long chunkStart = curPixel * bytespp;
        unsigned long curByte = curPixel * bytespp;
        unsigned char run_length = 1;
        bool raw = true;
        while (curPixel + run_length < nPixels && run_length < max_chunk_length) {
            bool succ_eq = true;
            for (int t = 0; succ_eq && t < bytespp; t++) {
                succ_eq = (data[curByte + t] == data[curByte + t + bytespp]);
            }
            curByte += bytespp;
            if (1 == run_length) {
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
        out.put(raw ? run_length - 1 : run_length + 127);
        if (!out.good()) {
            std::cerr << "Can't dump the tga file\n";
            return false;
        }

        out.write((char *) (data + chunkStart), (raw ? run_length * bytespp : bytespp));
        if (!out.good()) {
            std::cerr << "Can't dump the tga file\n";
            return false;
        }
    }
    return true;
}

TGAColor TGAImage::get(const int &x, const int &y) const {
    if (!data || x < 0 || y < 0 || x >= width || y >= height) {
        return TGAColor();
    }
    return TGAColor(data + (x + y * width) * bytespp, bytespp);
}

bool TGAImage::set(const int &x, const int &y, const TGAColor &c) {
    if (!data || x < 0 || y < 0 || x >= width || y >= height) {
        return false;
    }
    memcpy(data + (x + y * width) * bytespp, c.raw, bytespp);
    return true;
}

int TGAImage::get_bytespp() {
    return bytespp;
}

int TGAImage::get_width() {
    return width;
}

int TGAImage::get_height() {
    return height;
}

bool TGAImage::flip_horizontally() {
    if (!data)
        return false;
    int half = width >> 1;
    for (unsigned int i = 0; i < half; i++) {
        for (unsigned j = 0; j < height; j++) {
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
    unsigned long bytes_per_line = width * bytespp;
    auto *line = new unsigned char[bytes_per_line];
    int half = height >> 1;
    for (int j = 0; j < half; j++) {
        unsigned long l1 = j * bytes_per_line;
        unsigned long l2 = (height - 1 - j) * bytes_per_line;
        memmove((void *) line, (void *) (data + l1), bytes_per_line);
        memmove((void *) (data + l1), (void *) (data + l2), bytes_per_line);
        memmove((void *) (data + l2), (void *) line, bytes_per_line);
    }
    delete[] line;
    return true;
}

unsigned char *TGAImage::buffer() {
    return data;
}

void TGAImage::clear() {
    memset(data, 0, width * height * bytespp);
}

bool TGAImage::scale(const unsigned int &w, const unsigned int &h) {
    if (w <= 0 || h <= 0 || !data) return false;
    auto *tData = new unsigned char[w * h * bytespp];
    int nscanline = 0;
    int oscanline = 0;
    int erry = 0;
    unsigned long nlinebytes = w * bytespp;
    unsigned long olinebytes = width * bytespp;
    for (int j = 0; j < height; j++) {
        int errx = width - w;
        int nx = -bytespp;
        int ox = -bytespp;
        for (int i = 0; i < width; i++) {
            ox += bytespp;
            errx += w;
            while (errx >= (int) width) {
                errx -= width;
                nx += bytespp;
                memcpy(tData + nscanline + nx, data + oscanline + ox, bytespp);
            }
        }
        erry += h;
        oscanline += olinebytes;
        while (erry >= (int) height) {
            if (erry >= (int) height << 1) // it means we jump over a scanline
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