#include "scene.h"

Scene::Scene()
{
    // Scene starts empty.
}

void Scene::removeObject(size_t index)
{
    if (index < m_objects.size()) {
        m_objects.erase(m_objects.begin() + index);
    }
}
