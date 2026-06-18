#ifndef SIMULATION_WORKER_H
#define SIMULATION_WORKER_H

#include <QObject>
#include <vector>
#include "geometry.h"

class SimulationWorker : public QObject
{
    Q_OBJECT

public:
    explicit SimulationWorker(QObject *parent = nullptr);

public slots:
    void calculateMagneticField(const std::vector<Vec3>& coilPath, float current);

signals:
    void fieldLinesCalculated(const std::vector<FieldLine>& fieldLines);
    
private:
    std::vector<Vec3> m_lastCoilPath;
    std::vector<FieldLine> m_cachedUnitFieldLines;
};

#endif // SIMULATION_WORKER_H
