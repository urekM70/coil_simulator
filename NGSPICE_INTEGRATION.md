# Ngspice Integration Architecture

> **Note:** This document describes the *intended* integration architecture. Currently, the `NgspiceInterface` is mocked and simulates success rather than dynamically loading the library.

This document describes the intended integration of the ngspice circuit simulator into the application using the shared library (C API) approach.

## 1. Integration Architecture

The integration relies on a wrapper class `NgspiceInterface` that manages the interaction with `ngspice.dll` (Windows) or `libngspice.so` (Linux). The simulation runs asynchronously to avoid blocking the UI thread.

### Components

- **NgspiceInterface Class**:
  - **Responsibilities**:
    - Load and initialize the shared library via `QLibrary`.
    - Register callback functions (`SendChar`, `SendStat`, `SendData`, etc.).
    - Send commands (e.g., `circbyline`, `bg_run`, `bg_halt`).
    - Store simulation results in thread-safe containers.
  - **Callbacks**:
    - `cbSendData`: Captures simulation data (vectors) point-by-point as the simulation progresses.
    - `cbBGThreadRunning`: Tracks the running state of the background simulation thread.

- **Main Application**:
  - Instantiates `NgspiceInterface`.
  - Constructs a netlist string from actuator parameters.
  - Calls `loadCircuit(netlist)` and `runTransient(...)`.
  - Periodically polls `getResults()` or connects via signals to update plots/UI.

### Threading Model

Ngspice supports running in a background thread managed internally by the library (`bg_run`).
1.  **UI Thread**: Calls `runTransient`.
2.  **Ngspice Background Thread**: Executes the simulation loop.
3.  **Callbacks**: Called from the background thread.
    - **Critical Section**: Data access in `cbSendData` is protected by `std::mutex` to prevent race conditions when the UI reads results.

## 2. Example Circuit Definition

The application generates a netlist string representing the RL circuit of the actuator.

**Example Netlist:**

```spice
* Actuator Driver Circuit
* Voltage Source: Pulse from 0V to 24V
V1 1 0 PULSE(0 24 0 1u 1u 10m 20m)

* Switch (Modeled as Voltage Controlled Resistor or simple Resistor for now)
* R_switch 1 2 0.1

* Coil Resistance
R_coil 1 2 5.0

* Coil Inductance (Supplied externally/parametrically)
L_coil 2 0 10m

* Transient Analysis: Step 10us, Stop 50ms
.TRAN 10u 50m
.end
```

In the implementation, this string is built dynamically:

```cpp
std::stringstream netlist;
netlist << "* Generated Circuit\n";
netlist << "V1 1 0 " << voltageSourceString << "\n";
netlist << "R1 1 2 " << resistance << "\n";
netlist << "L1 2 0 " << inductance << "\n"; // Inductance from geometry
netlist << ".TRAN " << step << " " << stop << "\n";
netlist << ".end\n";
interface.loadCircuit(netlist.str());
```

## 3. Data Extraction (Current vs Time)

Data is extracted in real-time using the `SendData` callback mechanism provided by ngspice.

1.  **Callback Registration**:
    During initialization, `cbSendData` is registered with `ngSpice_Init`.

2.  **Data Packet**:
    Ngspice calls `cbSendData` with a `vecvaluesall` struct containing an array of `vecvalues`.

3.  **Parsing**:
    Inside the callback:
    - Iterate through the `vecvalues` array.
    - Identify the **time** vector (`vec->name == "time"`).
    - Identify the **current** vector (e.g., `vec->name == "i(v1)"` or branch current `vec->name == "v1#branch"`). Note: Current through a voltage source is typically available. For an inductor `L1`, the current might be `i(l1)` depending on save options.

4.  **Storage**:
    - Values are pushed into `std::vector<double>` containers within `m_currentResults`.
    - A mutex ensures thread safety.

5.  **Retrieval**:
    - The UI thread calls `getResults()` which locks the mutex and returns a copy or reference to the accumulated data for plotting.

```cpp
// Logic in cbSendData
for (int i = 0; i < data->veccount; ++i) {
    if (std::string(data->vecsa[i]->name) == "time") {
        results.time.push_back(data->vecsa[i]->creal);
    } else if (std::string(data->vecsa[i]->name) == "i(l1)") {
        results.vectors["current"].push_back(data->vecsa[i]->creal);
    }
}
```
