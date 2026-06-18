#include "simulation_controller.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

SimulationController::SimulationController(QObject *parent) : QObject(parent) {
}

SimulationController::~SimulationController() {
}

void SimulationController::setObject(ActuatorObject* electromagnet) {
    m_electromagnet = electromagnet;
}

void SimulationController::setSimulationMode(SimulationMode mode)
{
    m_simulationMode = mode;
}

void SimulationController::calculateStateAtTime(float time)
{
    if (!m_electromagnet) return;

    inductance = calculateInductance(position);

    float R_20 = 1e-9f;
    float voltage = 12.0f;
    float current_limit = 5.0f;
    float pulse_width = 10.0f;
    float mu0 = 4.0f * 3.14159f * 1e-7;
    float n_turns = 1.0f;
    float length = 0.01f; // 10mm default

    if (auto* coil = dynamic_cast<Coil*>(m_electromagnet)) {
        R_20 = coil->resistance;
        voltage = coil->voltage;
        current_limit = coil->current_limit;
        pulse_width = coil->pulse_width;
        n_turns = coil->num_turns;
        length = coil->coil_length / 1000.0f;
    }

    if (R_20 < 1e-9f) R_20 = 1e-9f;

    float alpha = 0.00393f;
    float R_hot = R_20 * (1.0f + alpha * (temperature - 20.0f));

    if (m_simulationMode == CONTINUOUS) {
        float tau = inductance / R_hot;
        if (tau < 1e-9) tau = 1e-9;
        current = (voltage / R_hot) * (1.0f - std::exp(-time / tau));
    } else { // PULSED
        if (time < (pulse_width / 1000.0f)) {
            float tau = inductance / R_hot;
            if (tau < 1e-9) tau = 1e-9;
            current = (voltage / R_hot) * (1.0f - std::exp(-time / tau));
        } else {
            float tau = inductance / R_hot;
            if (tau < 1e-9) tau = 1e-9;
            float I_peak = (voltage / R_hot) * (1.0f - std::exp(-(pulse_width / 1000.0f) / tau));
            float t_decay = time - (pulse_width / 1000.0f);
            current = I_peak * std::exp(-t_decay / tau);
        }
    }

    if (current > current_limit) current = current_limit;
    if (current < -current_limit) current = -current_limit;
    
    if (length < 0.001f) length = 0.001f;
    magnetic_field = mu0 * (n_turns / length) * current;

    if (current > 1000.0f) {
        qWarning() << "Warning: Current > 1kA. This is likely non-physical for standard wires.";
    }
    float thermal_energy = current * current * R_hot * time;
    if (thermal_energy > 10000.0f) {
        qWarning() << "Warning: High thermal energy dissipation.";
    }
    if (magnetic_field > 2.0f) {
        qWarning() << "Warning: Magnetic field > 2T. May not be structurally sound.";
    }
}

float SimulationController::calculateInductance(float /*pos*/) {
    if (!m_electromagnet) return 0.0f;
    
    float mu0 = 4.0f * 3.14159f * 1e-7;
    float n_turns = 1.0f;
    float length = 0.01f;
    float area = 0.0001f;

    if (auto* coil = dynamic_cast<Coil*>(m_electromagnet)) {
        n_turns = coil->num_turns;
        length = coil->coil_length / 1000.0f;
        float r_avg = (coil->inner_radius / 1000.0f) + (coil->wire_diameter / 2000.0f);
        area = 3.14159f * r_avg * r_avg;
    }

    if (length < 0.001f) length = 0.001f;
    float inductance = (mu0 * n_turns * n_turns * area) / length;
    return inductance;
}
