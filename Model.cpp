#include <utility>

//
// Created by ju5t on 29.01.19.
//

#include <fstream>
#include <sstream>

#include "Model.h"

Model::Model(const char *filename) : vertices(), faces(), norms(), uvs(), diffuseMap() {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) {
        std::cerr << "Failed to open file " << filename << '\n';
        return;
    }

    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line);

        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; ++i) {
                iss >> v[i];
            }
            vertices.push_back(v);
        } else if (!line.compare(0, 2, "f ")) {
            std::vector<Vec3i> f;
            Vec3i tmp;
            iss >> trash;
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
                for (int i = 0; i < 3; ++i) {
                    tmp[i]--;
                }
                f.push_back(tmp);
            }
            faces.push_back(f);
        } else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            Vec3f n;
            for (int i = 0; i < 3; i++)
                iss >> n[i];
            norms.push_back(n);
        } else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f uv;
            for (int i = 0; i < 2; i++)
                iss >> uv[i];
            uvs.push_back(uv);
        }
    }

    std::cerr << "# v# " << vertices.size() << " f# " << faces.size() << " vt# " << uvs.size() << " vn# "
              << norms.size() << std::endl;
    load_texture(filename, "_diffuse.tga", diffuseMap);
}

int Model::nVertices() const {
    return static_cast<int>(vertices.size());
}

int Model::nFaces() const {
    return static_cast<int>(faces.size());
}

Vec3f Model::get_vertex(const int &idx) const {
    return vertices[idx];
}

std::vector<int> Model::get_face(const int &idx) {
    std::vector<int> face;

    for (auto &i : faces[idx])
        face.push_back(i[0]);

    return face;
}

void Model::load_texture(std::string filename, std::string suffix, TGAImage &img) {
    size_t dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        std::string textureFile = filename.substr(0, dot) + suffix;

        bool is_ok = img.read_tga_file(textureFile);
        std::cerr << "texture file " << textureFile << " loading " << (is_ok ? "ok" : "failed") << std::endl;
        img.flip_vertically();
    }
}

Vec2i Model::get_uv(int iFace, int nVertex) {
    int idx = faces[iFace][nVertex][1];
    return Vec2i(static_cast<int>(uvs[idx].x * diffuseMap.get_width()),
                 static_cast<int>(uvs[idx].y * diffuseMap.get_height()));
}

TGAColor Model::get_diffuse(Vec2i uv) {
    return diffuseMap.get(uv.x, uv.y);
}
