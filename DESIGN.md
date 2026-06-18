# System Design: Electromechanical Coupling

> **Note:** This document outlines the *planned* system architecture. Currently, features such as full EM solving, Inductance Caching, and rigorous Circuit/Physics integration are in the prototype/analytical phase.

## 1. Data Flow Diagram

```mermaid
graph TD
    User[User Input] -->|Parameters| Geo[Geometry Engine]
    Geo -->|Mesh & Volume| EM[EM Solver]
    Geo -->|Movement| EM
    
    EM -->|Inductance L(x)| Cache[Inductance Cache]
    Cache -->|L(x), dL/dx| Circuit[Circuit Solver]
    
    Circuit -->|Current I(t)| Force[Force Calculation]
    Cache -->|dL/dx| Force
    
    Force -->|F_mag| Physics[Rigid Body Physics]
    Physics -->|Position x(t)| Geo
```

## 2. Components

### Geometry Engine
- **Input:** Actuator parameters (dimensions, turns, wire gauge).
- **Output:** 
  - Visual Mesh (for OpenGL).
  - Physics Mesh/Volume (for EM integration).
  - State: Position $x$ (plunger/mover position).

### EM Solver (Quasi-static)
- **Role:** Compute magnetic field energy and inductance for a given geometry configuration $x$.
- **Method:** 
  - Finite Element Method (FEM) or Magnetic Circuit Analysis (MCA).
  - For simple solenoids: Analytical approximations or pre-computed lookup tables.
- **Output:** Inductance $L(x)$.

### Inductance Cache
- **Role:** Avoid expensive EM solves every frame.
- **Strategy:** 
  - Store pairs $(x_i, L_i)$.
  - Interpolate $L(x)$ and $\frac{dL}{dx}$ using cubic splines.
  - Invalidate if geometry parameters change.

### Circuit Solver
- **Role:** Solve RL circuit equation: $V = IR + \frac{d}{dt}(Li) = IR + L\frac{di}{dt} + i\frac{dL}{dx}v$.
- **Input:** Voltage $V(t)$, Resistance $R$, $L(x)$, $\frac{dL}{dx}$, velocity $v$.
- **Output:** Current $I(t)$.

### Force Calculation
- **Method:** Energy Gradient.
- $F_{mag} = \frac{1}{2} I^2 \frac{dL}{dx}$.

## 3. Update Rules

1.  **Parameter Change (Edit Mode):**
    - User updates dimensions.
    - Clear **Inductance Cache**.
    - Re-generate visual mesh.

2.  **Physics Step (Simulation Mode):**
    - **Step 1:** Get current position $x$ and velocity $v$ from Physics Engine.
    - **Step 2 (EM Lookup):** 
      - Query Cache for $L(x)$. 
      - If missing, run EM Solver for $x$ (or nearby points) and update Cache.
      - Compute gradient $\frac{dL}{dx}$ from Cache (finite difference or spline derivative).
    - **Step 3 (Circuit Step):**
      - Integrate circuit ODE to find $I_{new}$.
    - **Step 4 (Force):**
      - Compute $F = \frac{1}{2} I^2 \frac{dL}{dx}$.
    - **Step 5 (Physics):**
      - Apply $F$ to rigid body.
      - Integrate motion equations to find $x_{new}, v_{new}$.

## 4. Caching Strategy

- **Lazy Evaluation:** Only compute $L(x)$ when the object moves to a new position $x$.
- **Spatial Hash / Grid:** Discretize the stroke (range of motion) into $N$ points.
- **Pre-computation (Optional):** If the simulation is expected to cover the full range, run a background thread to fill the cache for $x \in [0, \text{max\_stroke}]$.
- **Invalidation:** Any change to core geometry (radius, turns, material) clears the entire cache.
