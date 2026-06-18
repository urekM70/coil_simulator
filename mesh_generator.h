#ifndef MESH_GENERATOR_H
#define MESH_GENERATOR_H

#include "actuator_objects.h"
#include "geometry.h"

// Physics visualization
std::vector<FieldLine> generate_magnetic_field_lines(const std::vector<Vec3>& coilPath, float current);

#endif // MESH_GENERATOR_H
