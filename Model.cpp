//
// Created by ju5t on 29.01.19.
//

#include <fstream>
#include <sstream>

#include "Model.h"

Model::Model(const char *filename) : verts(), faces() {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail())
        return;

    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line);

        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; ++i) {
                iss >> v.raw[i];
            }
            verts.push_back(v);
        } else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            int iTrash, idx;
            iss >> trash;
            while (iss >> idx >> trash >> iTrash >> trash >> iTrash) {
                idx--;
                f.push_back(idx);
            }
            faces.push_back(f);
        }
    }

    std::cerr << "# v# " << verts.size() << " f# " << faces.size() << std::endl;
}

Model::~Model() {}

int Model::nVerts() {
    return static_cast<int>(verts.size());
}

int Model::nFaces() {
    return static_cast<int>(faces.size());
}

Vec3f Model::vert(int i) {
    return verts[i];
}

std::vector<int> Model::face(int idx) {
    return faces[idx];
}
