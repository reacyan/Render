#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), faces_() {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++) iss >> v.raw[i];
            verts_.push_back(v);
        } else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f uv;
            for (int i: {0, 1}) iss >> uv.raw[i];
            tex.push_back({uv.x, 1 - uv.y});
        } else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            std::vector<int> t;
            int itrash, idx,tex_ind;
            iss >> trash;
            while (iss >> idx >> trash >> tex_ind >> trash >> itrash) {
                idx--;
                f.push_back(idx);
                tex_ind--;
                t.push_back(tex_ind);
            }
            faces_.push_back(f);
            text_index_.push_back(t);
        }
        std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
    }
}

Model::~Model() {
}

int Model::nverts() {
    return (int) verts_.size();
}

int Model::nfaces() {
    return (int) faces_.size();
}

std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

int Model::ntext() {
    return (int) text_index_.size();
}

std::vector<int> Model::text(int idx) {
    return text_index_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec2f Model::uv(int i) {
    return tex[i];
}


