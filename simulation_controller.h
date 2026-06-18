#ifndef SIMULATION_CONTROLLER_H
#define SIMULATION_CONTROLLER_H

#include "actuator_objects.h"
#include <QObject>

class SimulationController : public QObject {
    Q_OBJECT

public:
    enum SimulationMode {
        CONTINUOUS,
        PULSED
    };

    explicit SimulationController(QObject *parent = nullptr);
    ~SimulationController();

    void setObject(ActuatorObject* electromagnet);
    void calculateStateAtTime(float time);

    // Physics State
    float position = 0.0f; // Mover position (m)
    float velocity = 0.0f; // Mover velocity (m/s)
    float current = 0.0f;  // Circuit current (A)
    float inductance = 0.0f; // Current L (H)
    float force = 0.0f;    // Magnetic force (N)
    float temperature = 20.0f; // Coil Temp (C)
    float magnetic_field = 0.0f; // Magnetic field B (T)

    SimulationMode m_simulationMode = CONTINUOUS;

public slots:
    void setSimulationMode(SimulationMode mode);

private:
    // Helpers
    float calculateInductance(float pos);

    ActuatorObject* m_electromagnet = nullptr;
};

#endif // SIMULATION_CONTROLLER_H
