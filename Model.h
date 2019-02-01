//
// Created by ju5t on 29.01.19.
//

#ifndef SIMPLESOFTWARERENDERER_MODEL_H
#define SIMPLESOFTWARERENDERER_MODEL_H

#include <vector>
#include "geometry.h"
#include "TGAImage.h"

class Model {
private:
    std::vector<Vec3f> vertices;
    std::vector<std::vector<Vec3i>> faces; // attention, this Vec3i means vertex/uv/normal
    std::vector<Vec3f> norms;
    std::vector<Vec2f> uvs;
    TGAImage diffuseMap;

    void load_texture(std::string filename, std::string suffix, TGAImage &img);

public:
    explicit Model(const char *filename);

    ~Model() = default;

    int nVertices() const;

    int nFaces() const;

    Vec3f get_vertex(const int &idx) const;

    std::vector<int> get_face(const int &idx);

    Vec2i get_uv(int iFace, int nVertex);

    Vec3f get_norm(int iFace, int nVertex);

    TGAColor get_diffuse(Vec2i uv);
};

#endif //SIMPLESOFTWARERENDERER_MODEL_H
