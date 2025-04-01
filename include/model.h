#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
    std::vector<Vec2f> tex;
    std::vector<Vec3f> verts_;
    std::vector<std::vector<int> > faces_;
    std::vector<std::vector<int> > text_index_;

public:
    Model(const char *filename);

    ~Model();

    int nverts();

    int nfaces();

    int ntext();

    Vec3f vert(int i);

    Vec2f uv(int i);

    std::vector<int> face(int idx);

    std::vector<int> text(int idx);
};

#endif //__MODEL_H__
