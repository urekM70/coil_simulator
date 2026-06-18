#ifndef ACTUATOR_OBJECTS_H
#define ACTUATOR_OBJECTS_H

#include "geometry.h"
#include <string>
#include <vector>
#include <utility> // For std::pair

// Parameter structure now holds a pointer for stability
using Parameter = std::pair<std::string, float*>;

// Abstract base class for all procedural objects
class ActuatorObject {
public:
    virtual ~ActuatorObject() = default;
    virtual std::string getName() const = 0;
    virtual std::vector<Parameter> getParameters() = 0;
    virtual TriangleMesh generateMesh() const = 0;
    virtual std::vector<Vec3> getPath() const { return {}; }
    virtual void onParameterChanged(const std::string& /*name*/) {}
};

// Parameters for a realistic wire-wound coil
struct Coil : public ActuatorObject {
    std::string name = "Coil";
    float inner_radius = 10.0f;      // Radius of the form/mandrel
    float wire_diameter = 1.0f;      // Thickness of the wire
    float wire_length = 1000.0f;     // Total length of the wire in mm
    float num_layers = 1.0f;         // Number of layers of wire
    int segments = 12;               // Cross-section resolution of the wire
    float num_turns = 100.0f;        // Number of turns
    float coil_length = 20.0f;       // Length of the coil in mm

    // Electrical properties
    float voltage = 12.0f;           // Volts
    float current_limit = 5.0f;      // Amps (Limit)
    float resistance = 0.0f;         // Ohms (Calculated)
    float current_actual = 0.0f;     // State variable for visualization
    float pulse_width = 10.0f;       // Pulse width in ms

    // Material properties
    const float rho = 1.68e-8;       // Ohm*m (Copper resistivity)
    const float mu0 = 4.0f * 3.14159f * 1e-7; // H/m

    Coil() {
        recalculateProperties();
    }

    std::string getName() const override { return name; }

    std::vector<Parameter> getParameters() override {
        return {
            {"Mandrel Radius", &inner_radius},
            {"Wire Diameter", &wire_diameter},
            {"Wire Length", &wire_length},
            {"Layers", &num_layers},
            {"Voltage (V)", &voltage},
            {"Current Limit (A)", &current_limit},
            {"Pulse Width (ms)", &pulse_width},
            {"Number of Turns", &num_turns},
            {"Coil Length (mm)", &coil_length},
            {"Resistance (Ohm)", &resistance}
        };
    }

    TriangleMesh generateMesh() const override;
    std::vector<Vec3> getPath() const override;
    
    void onParameterChanged(const std::string& /*name*/) override {
        recalculateProperties();
    }

private:
    void recalculateProperties() {
        if (wire_diameter < 0.001f) wire_diameter = 0.001f;
        if (num_layers < 1.0f) num_layers = 1.0f;
        if (num_turns < 1.0f) num_turns = 1.0f;
        if (coil_length < 0.1f) coil_length = 0.1f;
        
        float turns_per_layer = num_turns / num_layers;
        float pitch = coil_length / turns_per_layer;
        float total_len = 0.0f;
        
        for (int l = 0; l < (int)num_layers; ++l) {
            float r = inner_radius + wire_diameter / 2.0f + l * wire_diameter;
            float turn_len = std::sqrt(pow(2.0f * 3.14159f * r, 2) + pitch * pitch);
            total_len += turns_per_layer * turn_len;
        }
        
        // Add terminal lengths
        float terminal_len = inner_radius * 0.5f;
        if (terminal_len < wire_diameter) terminal_len = wire_diameter;
        total_len += 2.0f * terminal_len;
        
        wire_length = total_len;
        
        float L = wire_length / 1000.0f; // Convert mm to m
        float r_wire = (wire_diameter / 1000.0f) / 2.0f;
        float A = 3.14159f * r_wire * r_wire;
        resistance = (rho * L) / A;
        
        if (resistance < 1e-9f) resistance = 1e-9f;
    }
};

#endif // ACTUATOR_OBJECTS_H
