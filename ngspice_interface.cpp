#include "ngspice_interface.h"
#include <QLibrary>
#include <QDebug>
#include <iostream>
#include <cstring>
#include <vector>

// Define function pointers for ngspice API
typedef int (*ngSpice_Init_t)(void*, void*, void*, void*, void*, void*, void*);
typedef int (*ngSpice_Command_t)(const char*);
typedef bool (*ngSpice_running_t)();
typedef char** (*ngSpice_AllVecs_t)(char*);

// Global or static instance for callbacks
static NgspiceInterface* g_interface = nullptr;

// Mock structs for compilation without ngspice headers
struct vecvalues {
    char* name;
    double creal;
    double cimag;
    bool is_scale;
    bool is_complex;
};

struct vecvaluesall {
    int veccount;
    int vecindex;
    vecvalues** vecsa;
};

NgspiceInterface::NgspiceInterface() {
    g_interface = this;
}

NgspiceInterface::~NgspiceInterface() {
    if (g_interface == this) g_interface = nullptr;
    stop();
}

bool NgspiceInterface::initialize() {
    // In a real implementation, load the library here
    // QLibrary lib("ngspice");
    // if (!lib.load()) return false;
    
    // Resolve symbols...
    // ngSpice_Init = (ngSpice_Init_t)lib.resolve("ngSpice_Init");
    
    // For now, simulate success
    qDebug() << "Ngspice interface initialized (simulated)";
    
    // Call ngSpice_Init with our static callbacks
    // ngSpice_Init(cbSendChar, cbSendStat, cbControlledExit, cbSendData, cbSendInitData, cbBGThreadRunning, this);
    
    return true;
}

bool NgspiceInterface::sendCommand(const std::string& cmd) {
    // ngSpice_Command(cmd.c_str());
    qDebug() << "Ngspice Command:" << QString::fromStdString(cmd);
    return true;
}

void NgspiceInterface::loadCircuit(const std::string& netlist) {
    // Usually via "source" command or building line by line using "circbyline"
    // Here we simulate loading
    qDebug() << "Loading circuit:\n" << QString::fromStdString(netlist);
    
    // Split lines and send via "circbyline"
    // sendCommand("circbyline " + line);
}

void NgspiceInterface::runTransient(double step, double stop) {
    // Construct tran command
    std::string cmd = "tran " + std::to_string(step) + " " + std::to_string(stop);
    sendCommand("bg_run"); // Background run
    sendCommand(cmd);
}

void NgspiceInterface::stop() {
    if (m_isRunning) {
        sendCommand("bg_halt");
    }
}

bool NgspiceInterface::isRunning() const {
    return m_isRunning;
}

NgspiceInterface::SimulationResult NgspiceInterface::getResults() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_currentResults;
}

// Callbacks implementation

int NgspiceInterface::cbSendChar(char* text, int id, void* user_data) {
    // Log stdout/stderr
    // qInfo() << "ngspice:" << text;
    return 0;
}

int NgspiceInterface::cbSendStat(char* text, int id, void* user_data) {
    // Status update
    return 0;
}

int NgspiceInterface::cbControlledExit(int status, bool immediate, bool quit, int id, void* user_data) {
    // Handle unload/exit
    return 0;
}

int NgspiceInterface::cbSendData(vecvaluesall* data, int num_structs, int id, void* user_data) {
    // This is called for each time step with vector values
    NgspiceInterface* self = static_cast<NgspiceInterface*>(user_data);
    if (!self) return 0;

    std::lock_guard<std::mutex> lock(self->m_dataMutex);
    
    // 'data' contains an array of 'vecvalues'
    // One of them is likely 'time'
    
    for (int i = 0; i < data->veccount; ++i) {
        vecvalues* vec = data->vecsa[i];
        if (vec) {
            std::string name(vec->name);
            double val = vec->creal;
            
            if (name == "time") {
                self->m_currentResults.time.push_back(val);
            } else {
                self->m_currentResults.vectors[name].push_back(val);
            }
        }
    }
    
    return 0;
}

int NgspiceInterface::cbSendInitData(vecvaluesall* data, int id, void* user_data) {
    // Initialization data, vector names
    return 0;
}

int NgspiceInterface::cbBGThreadRunning(bool is_running, int id, void* user_data) {
    NgspiceInterface* self = static_cast<NgspiceInterface*>(user_data);
    if (self) {
        self->m_isRunning = is_running;
    }
    return 0;
}
