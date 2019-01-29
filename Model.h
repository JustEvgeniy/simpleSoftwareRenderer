//
// Created by ju5t on 29.01.19.
//

#ifndef SIMPLESOFTWARERENDERER_MODEL_H
#define SIMPLESOFTWARERENDERER_MODEL_H

#include <vector>
#include "geometry.h"

class Model {
private:
    std::vector<Vec3f> verts;
    std::vector<std::vector<int>> faces;
public:
    Model(const char *filename);

    ~Model();

    int nVerts();

    int nFaces();

    Vec3f vert(int i);

    std::vector<int> face(int idx);
};

#endif //SIMPLESOFTWARERENDERER_MODEL_H
