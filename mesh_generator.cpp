#include "mesh_generator.h" // Actually we might remove this file later if we move logic
#include "actuator_objects.h"
#include <cmath>
#include <vector>

const float PI = 3.14159265359f;

// Helper to generate a cylinder (solid or hollow)
// Made static so it's private to this translation unit but reusable
static void generate_cylinder_impl(TriangleMesh& mesh, float inner_radius, float outer_radius, float height, int segments) {
    float half_height = height / 2.0f;
    unsigned int base_vertex_index = mesh.vertices.size();

    // Generate vertices and normals for cylinder walls
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2.0f * PI;
        float cos_a = cos(angle);
        float sin_a = sin(angle);

        // Outer wall
        mesh.vertices.push_back({outer_radius * cos_a, -half_height, outer_radius * sin_a});
        mesh.vertices.push_back({outer_radius * cos_a, half_height, outer_radius * sin_a});
        mesh.normals.push_back({cos_a, 0, sin_a});
        mesh.normals.push_back({cos_a, 0, sin_a});

        // Inner wall (if applicable)
        if (inner_radius > 0) {
            mesh.vertices.push_back({inner_radius * cos_a, -half_height, inner_radius * sin_a});
            mesh.vertices.push_back({inner_radius * cos_a, half_height, inner_radius * sin_a});
            mesh.normals.push_back({-cos_a, 0, -sin_a});
            mesh.normals.push_back({-cos_a, 0, -sin_a});
        }
    }

    // Generate indices for cylinder walls
    unsigned int verts_per_ring = (inner_radius > 0) ? 4 : 2;
    for (unsigned int i = 0; i < (unsigned int)segments; ++i) {
        unsigned int current = base_vertex_index + i * verts_per_ring;
        unsigned int next = current + verts_per_ring;

        // Outer wall
        mesh.indices.push_back(current); mesh.indices.push_back(next + 1); mesh.indices.push_back(current + 1);
        mesh.indices.push_back(current); mesh.indices.push_back(next); mesh.indices.push_back(next + 1);

        // Inner wall
        if (inner_radius > 0) {
            mesh.indices.push_back(current + 2); mesh.indices.push_back(current + 3); mesh.indices.push_back(next + 3);
            mesh.indices.push_back(current + 2); mesh.indices.push_back(next + 3); mesh.indices.push_back(next + 2);
        }
    }

    // --- Caps ---
    base_vertex_index = mesh.vertices.size();
    unsigned int cap_verts_per_ring = (inner_radius > 0) ? 2 : 1;

    // Generate vertices and normals for caps
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2.0f * PI;
        float cos_a = cos(angle);
        float sin_a = sin(angle);
        
        // Top cap
        mesh.vertices.push_back({outer_radius * cos_a, half_height, outer_radius * sin_a});
        mesh.normals.push_back({0, 1, 0});
        if (inner_radius > 0) {
            mesh.vertices.push_back({inner_radius * cos_a, half_height, inner_radius * sin_a});
            mesh.normals.push_back({0, 1, 0});
        }

        // Bottom cap
        mesh.vertices.push_back({outer_radius * cos_a, -half_height, outer_radius * sin_a});
        mesh.normals.push_back({0, -1, 0});
        if (inner_radius > 0) {
            mesh.vertices.push_back({inner_radius * cos_a, -half_height, inner_radius * sin_a});
            mesh.normals.push_back({0, -1, 0});
        }
    }

    // Add center points for solid cylinder caps
    unsigned int top_center_idx = 0, bottom_center_idx = 0;
    if (inner_radius == 0) {
        top_center_idx = mesh.vertices.size();
        mesh.vertices.push_back({0, half_height, 0});
        mesh.normals.push_back({0, 1, 0});
        bottom_center_idx = mesh.vertices.size();
        mesh.vertices.push_back({0, -half_height, 0});
        mesh.normals.push_back({0, -1, 0});
    }

    // Generate indices for caps
    for (unsigned int i = 0; i < (unsigned int)segments; ++i) {
        unsigned int current = base_vertex_index + i * cap_verts_per_ring * 2;
        unsigned int next = current + cap_verts_per_ring * 2;

        if (inner_radius > 0) {
            // Top cap
            mesh.indices.push_back(current); mesh.indices.push_back(next); mesh.indices.push_back(current + 1);
            mesh.indices.push_back(next); mesh.indices.push_back(next + 1); mesh.indices.push_back(current + 1);
            // Bottom cap
            unsigned int bottom_offset = cap_verts_per_ring;
            mesh.indices.push_back(current + bottom_offset); mesh.indices.push_back(current + bottom_offset + 1); mesh.indices.push_back(next + bottom_offset);
            mesh.indices.push_back(next + bottom_offset); mesh.indices.push_back(current + bottom_offset + 1); mesh.indices.push_back(next + bottom_offset + 1);
        } else {
            // Top cap (fan)
            mesh.indices.push_back(current); mesh.indices.push_back(top_center_idx); mesh.indices.push_back(next);
            // Bottom cap (fan)
            unsigned int bottom_offset = 1;
            mesh.indices.push_back(current + bottom_offset); mesh.indices.push_back(next + bottom_offset); mesh.indices.push_back(bottom_center_idx);
        }
    }
}

// Implement member functions for ActuatorObjects

// Helper to compute parallel transport frame (Rotation Minimizing Frame)
static void compute_frames(const std::vector<Vec3>& path, std::vector<Vec3>& tangents, std::vector<Vec3>& normals, std::vector<Vec3>& binormals) {
    if (path.size() < 2) return;
    
    tangents.resize(path.size());
    normals.resize(path.size());
    binormals.resize(path.size());
    
    // 1. Compute Tangents
    // First point
    tangents[0] = path[1] - path[0];
    tangents[0].normalize();
    // Middle points
    for(size_t i = 1; i < path.size() - 1; ++i) {
        Vec3 t1 = path[i] - path[i-1];
        t1.normalize();
        Vec3 t2 = path[i+1] - path[i];
        t2.normalize();
        tangents[i] = t1 + t2; // Average for smooth tangent
        tangents[i].normalize();
    }
    // Last point
    tangents.back() = path.back() - path[path.size()-2];
    tangents.back().normalize();
    
    // 2. Initial Frame
    Vec3 t0 = tangents[0];
    Vec3 v = {0, 1, 0};
    if (std::abs(dot(t0, v)) > 0.9f) v = {1, 0, 0};
    normals[0] = cross(t0, v);
    normals[0].normalize();
    binormals[0] = cross(t0, normals[0]);
    binormals[0].normalize();
    
    // 3. Propagate Frame
    for(size_t i = 0; i < path.size() - 1; ++i) {
        Vec3 t1 = tangents[i];
        Vec3 t2 = tangents[i+1];
        Vec3 n1 = normals[i];
        
        Vec3 axis = cross(t1, t2);
        float len = axis.length();
        if (len < 0.0001f) {
            normals[i+1] = n1;
        } else {
            axis.normalize();
            float dot_val = dot(t1, t2);
            if (dot_val > 1.0f) dot_val = 1.0f;
            if (dot_val < -1.0f) dot_val = -1.0f;
            float angle = acos(dot_val);
            
            // Rotate n1 around axis
            float c = cos(angle);
            float s = sin(angle);
            
            Vec3 term1 = n1 * c;
            Vec3 term2 = cross(axis, n1) * s;
            Vec3 term3 = axis * (dot(axis, n1) * (1.0f - c));
            
            normals[i+1] = term1 + term2 + term3;
        }
        
        // Re-orthogonalize
        normals[i+1] = normals[i+1] - t2 * dot(t2, normals[i+1]);
        normals[i+1].normalize();
        
        binormals[i+1] = cross(t2, normals[i+1]);
    }
}

// Helper to generate the path for a multi-layer coil
static std::vector<Vec3> generate_coil_path(float mandrel_radius, float wire_diameter, float coil_length, float num_turns, float num_layers_f) {
    std::vector<Vec3> path;
    if (coil_length <= 0 || wire_diameter <= 0 || mandrel_radius < 0 || num_turns <= 0) return path;
    
    int num_layers = (int)num_layers_f;
    if (num_layers < 1) num_layers = 1;

    float turns_per_layer = num_turns / num_layers;
    float layer_height = coil_length;

    float terminal_len = mandrel_radius * 0.5f;
    if (terminal_len < wire_diameter) terminal_len = wire_diameter;
    
    // Start Terminal (Radial Outward)
    float R0 = mandrel_radius + wire_diameter/2.0f;
    path.push_back({R0 + terminal_len, -layer_height/2.0f, 0});
    path.push_back({R0, -layer_height/2.0f, 0});
    
    float current_theta = 0.0f;
    
    for(int l = 0; l < num_layers; ++l) {
        float r = mandrel_radius + wire_diameter/2.0f + l*wire_diameter;
        bool up = (l % 2 == 0);
        float start_y = up ? -layer_height/2.0f : layer_height/2.0f;
        float end_y = up ? layer_height/2.0f : -layer_height/2.0f;
        
        int steps = (int)(turns_per_layer * 36); 
        if (steps < 2) steps = 2;
        
        // Helix
        for(int i = 1; i <= steps; ++i) {
             float t = (float)i / steps;
             float theta = current_theta + t * (turns_per_layer * 2.0f * PI);
             float y = start_y + t * (end_y - start_y);
             path.push_back({static_cast<float>(r * cos(theta)), y, static_cast<float>(r * sin(theta))});
        }
        current_theta += turns_per_layer * 2.0f * PI;
        
        // Transition
        if (l < num_layers - 1) {
            float next_r = mandrel_radius + wire_diameter/2.0f + (l+1)*wire_diameter;
            int trans_steps = 9; 
            float trans_angle = PI / 2.0f;
            for(int i = 1; i <= trans_steps; ++i) {
                float t = (float)i / trans_steps;
                float theta = current_theta + t * trans_angle;
                float r_trans = r + t * (next_r - r);
                path.push_back({static_cast<float>(r_trans * cos(theta)), end_y, static_cast<float>(r_trans * sin(theta))});
            }
            current_theta += trans_angle;
        }
    }
    
    // End Terminal (Radial Outward)
    Vec3 last = path.back();
    Vec3 radial = {last.x, 0, last.z};
    float radial_len = radial.length();
    if (radial_len > 0.0001f) {
        radial = radial * (1.0f / radial_len);
    } else {
        radial = {1, 0, 0};
    }
    Vec3 term_end = last + radial * terminal_len;
    path.push_back(term_end);
    
    return path;
}

std::vector<Vec3> Coil::getPath() const {
    return generate_coil_path(inner_radius, wire_diameter, coil_length, num_turns, num_layers);
}

TriangleMesh Coil::generateMesh() const {
    TriangleMesh mesh;
    std::vector<Vec3> path = getPath();
    if (path.empty()) return mesh;

    // Compute Frames
    std::vector<Vec3> tangents, normals, binormals;
    compute_frames(path, tangents, normals, binormals);
    
    // Generate Mesh
    float wire_radius = wire_diameter / 2.0f;
    unsigned int base_idx = mesh.vertices.size();
    
    for(size_t i = 0; i < path.size(); ++i) {
        Vec3 center = path[i];
        Vec3 right = normals[i]; 
        Vec3 up = binormals[i];
        
        for (int j = 0; j <= segments; ++j) {
            float phi = (float)j / segments * 2.0f * PI;
            Vec3 local_pos = right * (cos(phi) * wire_radius) + up * (sin(phi) * wire_radius);
            Vec3 pos = center + local_pos;
            Vec3 normal = local_pos;
            normal.normalize();
            
            mesh.vertices.push_back(pos);
            mesh.normals.push_back(normal);
        }
    }
    
    // Indices
    for(size_t i = 0; i < path.size() - 1; ++i) {
        for (int j = 0; j < segments; ++j) {
            int current = base_idx + i * (segments + 1) + j;
            int next = current + (segments + 1);
            
            mesh.indices.push_back(current);
            mesh.indices.push_back(next);
            mesh.indices.push_back(current + 1);
            
            mesh.indices.push_back(current + 1);
            mesh.indices.push_back(next);
            mesh.indices.push_back(next + 1);
        }
    }
    return mesh;
}

// Calculate Biot-Savart B field at pos (Result in Tesla)
// Current I in Amps. Path in mm. Pos in mm.
static Vec3 calculate_b_field(const std::vector<Vec3>& path, Vec3 pos, float current) {
    Vec3 B = {0, 0, 0};
    // Factor = 1e-7 * I * 1e3 = 1e-4 * I
    float factor = 1e-4f * current;
    
    for (size_t i = 0; i < path.size() - 1; ++i) {
        Vec3 p1 = path[i];
        Vec3 p2 = path[i+1];
        Vec3 dl = p2 - p1;
        Vec3 mid = (p1 + p2) * 0.5f;
        Vec3 r = pos - mid;
        float dist_sq = dot(r, r);
        float dist = std::sqrt(dist_sq);
        if (dist < 0.001f) continue; 
        
        Vec3 cross_prod = cross(dl, r);
        B = B + cross_prod * (1.0f / (dist_sq * dist));
    }
    return B * factor;
}

std::vector<FieldLine> generate_magnetic_field_lines(const std::vector<Vec3>& coilPath, float current) {
    std::vector<FieldLine> lines;
    if (coilPath.empty()) return lines;
    
    // Bounds
    float max_r = 0;
    float min_r = 1e9f;
    for(const auto& p : coilPath) {
        float r = std::sqrt(p.x*p.x + p.z*p.z);
        if (r > max_r) max_r = r;
        if (r < min_r) min_r = r;
    }
    if (min_r > max_r) min_r = max_r * 0.8f;
    
    int num_slices = 8;
    int seeds_inside = 4;
    int seeds_outside = 2;
    
    for (int k = 0; k < num_slices; ++k) {
        float phi = k * (2.0f * PI / num_slices);
        Vec3 dir = {static_cast<float>(cos(phi)), 0, static_cast<float>(sin(phi))};
        
        // Seeds inside the coil (strong field)
        for (int i = 0; i < seeds_inside; ++i) {
            float t = (float)i / seeds_inside; 
            float r_start = t * min_r * 0.85f; // Keep seeds mostly inside the core
            Vec3 start = dir * r_start;
            if (i == 0 && k > 0) continue; // Avoid duplicating the center seed
            
            // Forward trace
            std::vector<Vec3> points_fwd;
            std::vector<float> mag_fwd;
            
            Vec3 curr = start;
            points_fwd.push_back(curr);
            Vec3 b_start = calculate_b_field(coilPath, curr, current);
            mag_fwd.push_back(b_start.length());
            
            int steps = 150;
            float dt = max_r * 0.08f;
            
            for(int s=0; s<steps; ++s) {
                Vec3 B = calculate_b_field(coilPath, curr, current);
                float mag = B.length();
                if (mag < 1e-9f) break;
                
                Vec3 B_dir = B; 
                B_dir.normalize();
                
                curr = curr + B_dir * dt;
                points_fwd.push_back(curr);
                mag_fwd.push_back(mag);
                
                if (curr.length() > max_r * 4.0f) break;
            }
            
            // Backward trace
            std::vector<Vec3> points_bwd;
            std::vector<float> mag_bwd;
            curr = start;
            
            for(int s=0; s<steps; ++s) {
                Vec3 B = calculate_b_field(coilPath, curr, current);
                float mag = B.length();
                if (mag < 1e-9f) break;
                
                Vec3 B_dir = B;
                B_dir.normalize();
                
                curr = curr - B_dir * dt;
                points_bwd.push_back(curr);
                mag_bwd.push_back(mag);
                
                if (curr.length() > max_r * 4.0f) break;
            }
            
            // Merge: reverse backward + forward
            FieldLine line;
            for(int j=points_bwd.size()-1; j>=0; --j) {
                line.points.push_back(points_bwd[j]);
                line.intensity.push_back(mag_bwd[j]);
            }
            for(size_t j=0; j<points_fwd.size(); ++j) {
                line.points.push_back(points_fwd[j]);
                line.intensity.push_back(mag_fwd[j]);
            }
            lines.push_back(line);
        }
        
        // Seeds outside the coil (return flux)
        for (int i = 0; i < seeds_outside; ++i) {
            float t = (float)(i + 1) / (seeds_outside + 1); 
            float r_start = max_r * 1.1f + t * max_r * 0.8f;
            Vec3 start = dir * r_start;
            
            // Forward trace
            std::vector<Vec3> points_fwd;
            std::vector<float> mag_fwd;
            
            Vec3 curr = start;
            points_fwd.push_back(curr);
            Vec3 b_start = calculate_b_field(coilPath, curr, current);
            mag_fwd.push_back(b_start.length());
            
            int steps = 150;
            float dt = max_r * 0.08f;
            
            for(int s=0; s<steps; ++s) {
                Vec3 B = calculate_b_field(coilPath, curr, current);
                float mag = B.length();
                if (mag < 1e-9f) break;
                
                Vec3 B_dir = B; 
                B_dir.normalize();
                
                curr = curr + B_dir * dt;
                points_fwd.push_back(curr);
                mag_fwd.push_back(mag);
                
                if (curr.length() > max_r * 4.0f) break;
            }
            
            // Backward trace
            std::vector<Vec3> points_bwd;
            std::vector<float> mag_bwd;
            curr = start;
            
            for(int s=0; s<steps; ++s) {
                Vec3 B = calculate_b_field(coilPath, curr, current);
                float mag = B.length();
                if (mag < 1e-9f) break;
                
                Vec3 B_dir = B;
                B_dir.normalize();
                
                curr = curr - B_dir * dt;
                points_bwd.push_back(curr);
                mag_bwd.push_back(mag);
                
                if (curr.length() > max_r * 4.0f) break;
            }
            
            // Merge: reverse backward + forward
            FieldLine line;
            for(int j=points_bwd.size()-1; j>=0; --j) {
                line.points.push_back(points_bwd[j]);
                line.intensity.push_back(mag_bwd[j]);
            }
            for(size_t j=0; j<points_fwd.size(); ++j) {
                line.points.push_back(points_fwd[j]);
                line.intensity.push_back(mag_fwd[j]);
            }
            lines.push_back(line);
        }
    }
    return lines;
}

// Keep the old standalone functions for compatibility if needed, but they just call the member functions now
// or we can remove them if we update all call sites. 
// For now, I'll update the headers to remove the standalone function declarations and rely on the classes.
