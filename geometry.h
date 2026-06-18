#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "vec3.h"
#include <vector>

struct TriangleMesh {
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<unsigned int> indices;
};

struct FieldLine {
    std::vector<Vec3> points;
    std::vector<float> intensity;
};

#endif // GEOMETRY_H
