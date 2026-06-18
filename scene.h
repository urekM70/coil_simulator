#ifndef SCENE_H
#define SCENE_H

#include "actuator_objects.h"
#include <vector>
#include <memory>

class Scene
{
public:
    Scene();

    void removeObject(size_t index);

    // Scene now holds a polymorphic list of objects
    std::vector<std::unique_ptr<ActuatorObject>> m_objects;
    
    // For legacy/direct access during refactor (optional, but cleaner to access via m_objects)
    // Core m_core;
    // Tube m_tube;
    // Coil m_coil;
};

#endif // SCENE_H
