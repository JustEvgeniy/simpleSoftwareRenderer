//
// Created by ju5t on 29.01.19.
//

#ifndef SIMPLESOFTWARERENDERER_MODEL_H
#define SIMPLESOFTWARERENDERER_MODEL_H

#include <vector>
#include "geometry.h"

class Model {
private:
    std::vector<Vec3f> vertices;
    std::vector<std::vector<int>> faces;
public:
    explicit Model(const char *filename);

    ~Model() = default;

    int nVertices() const;

    int nFaces() const;

    Vec3f get_vertex(const int &idx) const;

    std::vector<int> get_face(const int &idx) const;
};

#endif //SIMPLESOFTWARERENDERER_MODEL_H
