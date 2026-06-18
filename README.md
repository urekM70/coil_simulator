# Coil Simulator

A real-time electromechanical coupling simulator framework for actuators and solenoids, built with C++20, Qt, and OpenGL, with planned Ngspice integration.

This application provides a foundation for simulating the multiphysics interactions of an electromagnetic actuator, currently focusing on analytical transient circuit equations, analytical inductance calculation, and 3D visualization.

![Simulation Demo](https://via.placeholder.com/800x450.png?text=Add+a+Screenshot+or+GIF+of+the+simulator+here)

## Features
* **Real-time 3D Visualization:** Custom OpenGL rendering engine for visualizing the coil and actuator mesh geometry, as well as generated magnetic field lines.
* **Transient Circuit Simulation (WIP):** Currently uses analytical RL circuit equations. Includes a mock/skeleton interface for the [Ngspice](http://ngspice.sourceforge.net/) circuit simulator, planned to solve complex RL circuit equations asynchronously via a shared C-API library.
* **Physics & Inductance Engine (WIP):** Calculates inductance analytically with plans to implement spatial inductance caching and cubic spline interpolation to calculate $L(x)$ and magnetic force ($F = \frac{1}{2} I^2 \frac{dL}{dx}$).
* **Multithreaded Architecture:** Strict separation between the Qt UI event loop and heavy background simulation workers (e.g., magnetic field generation) to ensure smooth performance.

## Architecture & Data Flow

```mermaid
graph TD
    User[User Input] -->|Geometry Params| Geo[Geometry Engine]
    Geo -->|Mesh & Volume| EM[EM Solver]
    
    EM -->|Inductance L(x)| Cache[Inductance Cache]
    Cache -->|L(x), dL/dx| Circuit[Ngspice Circuit Solver]
    
    Circuit -->|Current I(t)| Force[Force Calculation]
    Cache -->|dL/dx| Force
    
    Force -->|F_mag| Physics[Rigid Body Physics]
    Physics -->|Position x(t)| Geo
```

## Dependencies & Requirements
* **C++20** compiler
* **Qt 6** (Core, GUI, Widgets, OpenGLWidgets)
* **Ngspice shared library** (`ngspice.dll` on Windows or `libngspice.so` on Linux)

## Build Instructions
1. Clone the repository.
2. Open `coil_simulator.pro` in Qt Creator.
3. Ensure the Ngspice shared library is installed and accessible in your system/build path.
4. Build and run via Qt Creator or `qmake && make`.

## About This Project
This project was built as a hands-on exploration into C++, Qt, and multiphysics simulations. I utilized AI coding assistants to help scaffold boilerplate and draft core logic, which allowed me to focus on high-level system architecture, integrating third-party C libraries (Ngspice), and understanding C++ memory management, threading, and data types in a real-time context.
