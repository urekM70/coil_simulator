#include "simulation_worker.h"
#include "mesh_generator.h"
#include <algorithm>
#include <cmath>

SimulationWorker::SimulationWorker(QObject *parent) : QObject(parent)
{
}

void SimulationWorker::calculateMagneticField(const std::vector<Vec3>& coilPath, float current)
{
    // Check if the path changed
    bool pathChanged = false;
    if (coilPath.size() != m_lastCoilPath.size()) {
        pathChanged = true;
    } else {
        for (size_t i = 0; i < coilPath.size(); ++i) {
            // Check for geometric equality
            if (std::abs(coilPath[i].x - m_lastCoilPath[i].x) > 1e-4f ||
                std::abs(coilPath[i].y - m_lastCoilPath[i].y) > 1e-4f ||
                std::abs(coilPath[i].z - m_lastCoilPath[i].z) > 1e-4f) {
                pathChanged = true;
                break;
            }
        }
    }

    if (pathChanged || m_cachedUnitFieldLines.empty()) {
        m_lastCoilPath = coilPath;
        // Calculate field lines for a nominal 1.0A current
        m_cachedUnitFieldLines = generate_magnetic_field_lines(coilPath, 1.0f);
    }

    // Scale the cached field lines by the actual current
    std::vector<FieldLine> scaledLines = m_cachedUnitFieldLines;
    
    // Direction of the current can reverse the field
    bool reverse = (current < 0.0f);
    float abs_current = std::abs(current);
    
    for (auto& line : scaledLines) {
        for (auto& intensity : line.intensity) {
            // The calculated base was for 1.0A, so we just multiply by absolute current
            intensity *= abs_current;
        }
        
        // If current is negative, the field lines point in the opposite direction.
        // We reverse the point order to flip the visual flow of the line.
        if (reverse) {
            std::reverse(line.points.begin(), line.points.end());
            std::reverse(line.intensity.begin(), line.intensity.end());
        }
    }

    emit fieldLinesCalculated(scaledLines);
}
