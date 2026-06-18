#ifndef NGSPICE_INTERFACE_H
#define NGSPICE_INTERFACE_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

// Forward declaration of ngspice structs (simplified)
struct vecvaluesall;

class NgspiceInterface {
public:
    struct SimulationResult {
        std::vector<double> time;
        std::map<std::string, std::vector<double>> vectors; // vector name -> values
    };

    NgspiceInterface();
    ~NgspiceInterface();

    // Initialize the library
    bool initialize();

    // Run a command (e.g. "bg_run", "run", "source ...")
    bool sendCommand(const std::string& cmd);

    // Stop current simulation
    void stop();

    // Load a circuit netlist
    // Components:
    // - Voltage Source (V): "V1 1 0 PULSE..."
    // - Resistor (R): "R1 1 2 10"
    // - Inductor (L): "L1 2 0 10m"
    // - Capacitor (C): "C1 2 0 1u"
    // - Switch (S/W): "S1 ..."
    void loadCircuit(const std::string& netlist);

    // Run transient analysis
    // step: time step
    // stop: stop time
    void runTransient(double step, double stop);

    // Check if running
    bool isRunning() const;

    // Get latest results
    SimulationResult getResults();

    // Callbacks (static, directed to instance)
    static int cbSendChar(char* text, int id, void* user_data);
    static int cbSendStat(char* text, int id, void* user_data);
    static int cbControlledExit(int status, bool immediate, bool quit, int id, void* user_data);
    static int cbSendData(vecvaluesall* data, int num_structs, int id, void* user_data);
    static int cbSendInitData(vecvaluesall* data, int id, void* user_data);
    static int cbBGThreadRunning(bool is_running, int id, void* user_data);

private:
    std::atomic<bool> m_isRunning{false};
    std::mutex m_dataMutex;
    SimulationResult m_currentResults;

    // Helper to append data safely
    void appendData(const std::string& name, double value);
    void appendTime(double t);
};

#endif // NGSPICE_INTERFACE_H
