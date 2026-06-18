#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include "scene.h"
#include "actuator_objects.h"
#include "simulation_controller.h"
#include "simulation_worker.h"

// Forward declarations
class OpenGLWidget;
class QLineEdit;
class QListWidget;
class QVBoxLayout;
class QRadioButton;
class QDoubleSpinBox;
class QSlider;
class QLabel;
class QThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onObjectSelected(int index);
    void updateParametersFromUI();
    void onAddObject();
    void onObjectListContextMenu(const QPoint& pos);
    void onRemoveObject();
    void updateSimulationMode();
    void onTimeSliderChanged(int value);
    void onFieldLinesCalculated(const std::vector<FieldLine>& fieldLines);

signals:
    void calculateFieldLines(const std::vector<Vec3>& coilPath, float current);

private:
    void setupUi();
    void createMenus();
    void populateObjectList();
    void rebuildScene();
    void refreshParameterUI();
    void updateSimulationTime(float time);

    Scene m_scene;
    ActuatorObject* m_selectedObject = nullptr;
    SimulationController m_simulation;
    SimulationWorker* m_worker;
    QThread* m_workerThread;

    // UI Widgets
    OpenGLWidget* openGLWidget;
    QWidget* parameterEditor;
    QListWidget* objectList;
    QVBoxLayout* parameterLayout; 
    
    // Store current parameter widgets
    QVector<QLineEdit*> m_paramEdits;

    // Simulation UI
    QRadioButton* m_continuousModeRadio;
    QRadioButton* m_pulsedModeRadio;
    QDoubleSpinBox* m_pulseWidthSpinBox;
    QSlider* m_timeSlider;
    QLabel* m_timeLabel;
    QLabel* m_magneticFieldLabel;
};

#endif // MAINWINDOW_H
