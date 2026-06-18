#include "mainwindow.h"
#include "openglwidget.h"
#include "addobjectdialog.h"
#include "actuator_objects.h"
#include "mesh_generator.h"
#include <QSplitter>
#include <QListWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QRadioButton>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QLabel>
#include <QThread>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi();
    createMenus();
    connect(openGLWidget, &OpenGLWidget::initialized, this, &MainWindow::rebuildScene);

    m_worker = new SimulationWorker;
    m_workerThread = new QThread;
    m_worker->moveToThread(m_workerThread);

    connect(this, &MainWindow::calculateFieldLines, m_worker, &SimulationWorker::calculateMagneticField);
    connect(m_worker, &SimulationWorker::fieldLinesCalculated, this, &MainWindow::onFieldLinesCalculated);

    m_workerThread->start();
}

MainWindow::~MainWindow()
{
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_workerThread;
    delete m_worker;
}

void MainWindow::setupUi()
{
    objectList = new QListWidget;
    connect(objectList, &QListWidget::currentRowChanged, this, &MainWindow::onObjectSelected);
    objectList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(objectList, &QListWidget::customContextMenuRequested, this, &MainWindow::onObjectListContextMenu);

    openGLWidget = new OpenGLWidget;

    parameterEditor = new QWidget;
    parameterLayout = new QVBoxLayout(parameterEditor);
    
    QGroupBox* simGroup = new QGroupBox("Simulation Mode");
    QVBoxLayout* simLayout = new QVBoxLayout;
    m_continuousModeRadio = new QRadioButton("Continuous");
    m_pulsedModeRadio = new QRadioButton("Pulsed");
    m_pulseWidthSpinBox = new QDoubleSpinBox;
    m_pulseWidthSpinBox->setSuffix(" ms");
    m_pulseWidthSpinBox->setRange(0.1, 10000.0);
    simLayout->addWidget(m_continuousModeRadio);
    simLayout->addWidget(m_pulsedModeRadio);
    simLayout->addWidget(m_pulseWidthSpinBox);
    simGroup->setLayout(simLayout);
    
    m_timeSlider = new QSlider(Qt::Horizontal);
    m_timeSlider->setRange(0, 1000);
    m_timeLabel = new QLabel("Time: 0.0s");
    m_magneticFieldLabel = new QLabel("B-Field: 0.0T");

    parameterLayout->addWidget(simGroup);
    parameterLayout->addWidget(m_timeSlider);
    parameterLayout->addWidget(m_timeLabel);
    parameterLayout->addWidget(m_magneticFieldLabel);
    parameterLayout->addStretch();
    
    connect(m_continuousModeRadio, &QRadioButton::toggled, this, &MainWindow::updateSimulationMode);
    connect(m_pulsedModeRadio, &QRadioButton::toggled, this, &MainWindow::updateSimulationMode);
    connect(m_timeSlider, &QSlider::valueChanged, this, &MainWindow::onTimeSliderChanged);

    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    QSplitter *rightSplitter = new QSplitter(Qt::Horizontal, mainSplitter);

    rightSplitter->addWidget(openGLWidget);
    rightSplitter->addWidget(parameterEditor);
    rightSplitter->setSizes({600, 200});

    mainSplitter->addWidget(objectList);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setSizes({150, 850});

    setCentralWidget(mainSplitter);
    setWindowTitle(tr("Procedural EM Actuators"));
    resize(1280, 720);
    
    // Initial selection is nothing.
    onObjectSelected(-1);
    
    m_continuousModeRadio->setChecked(true);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close);

    QMenu *insertMenu = menuBar()->addMenu(tr("&Insert"));
    QAction *addObjectAction = insertMenu->addAction(tr("&Add Object..."));
    connect(addObjectAction, &QAction::triggered, this, &MainWindow::onAddObject);

    QMenu *simMenu = menuBar()->addMenu(tr("&Simulation"));
    simMenu->addAction(tr("&Start"), [this]() {
        updateSimulationTime(0);
    });
}

void MainWindow::populateObjectList()
{
    objectList->blockSignals(true);
    objectList->clear();
    for (const auto& obj : m_scene.m_objects) {
        objectList->addItem(QString::fromStdString(obj->getName()));
    }
    objectList->blockSignals(false);
}

void MainWindow::onObjectSelected(int index)
{
    // Clear existing parameter widgets, but not the simulation group box
    QLayoutItem *child;
    while (parameterLayout->count() > 5) { // Keep sim group and stretch
        child = parameterLayout->takeAt(4);
        delete child->widget();
        delete child;
    }
    m_paramEdits.clear();
    m_selectedObject = nullptr;

    if (index < 0 || (size_t)index >= m_scene.m_objects.size()) {
        return;
    }

    m_selectedObject = m_scene.m_objects[index].get();
    
    QGroupBox* groupBox = new QGroupBox(QString::fromStdString(m_selectedObject->getName()));
    QFormLayout* formLayout = new QFormLayout;

    for (const auto& param : m_selectedObject->getParameters()) {
        QLineEdit* edit = new QLineEdit(QString::number(*param.second));
        formLayout->addRow(QString::fromStdString(param.first), edit);
        m_paramEdits.append(edit);
        
        std::string paramName = param.first;
        connect(edit, &QLineEdit::editingFinished, [this, paramName](){
            updateParametersFromUI();
            if (m_selectedObject) {
                m_selectedObject->onParameterChanged(paramName);
                refreshParameterUI();
                updateSimulationTime(m_timeSlider->value() / 1000.0f);
            }
        });
    }

    groupBox->setLayout(formLayout);
    parameterLayout->insertWidget(4, groupBox);

    // Update simulation parameters based on selected electromagnet (Coil)
    ActuatorObject* em = m_selectedObject;
    
    auto params = em->getParameters();
    for (const auto& p : params) {
        if (p.first == "Pulse Width (ms)") {
            m_pulseWidthSpinBox->setValue(*(p.second));
            connect(m_pulseWidthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, p](double d){
                *(p.second) = d;
                updateSimulationTime(m_timeSlider->value() / 1000.0f);
            });
            break;
        }
    }
}

void MainWindow::updateParametersFromUI()
{
    if (!m_selectedObject) return;

    auto params = m_selectedObject->getParameters();
    for(size_t i = 0; i < params.size(); ++i) {
        if ((int)i < m_paramEdits.size()) {
            *(params[i].second) = m_paramEdits[i]->text().toFloat();
        }
    }
    
    rebuildScene();
}

void MainWindow::onObjectListContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = objectList->itemAt(pos);
    if (!item) return;

    QMenu contextMenu(this);
    QAction* deleteAction = contextMenu.addAction(tr("Delete"));
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onRemoveObject);
    contextMenu.exec(objectList->mapToGlobal(pos));
}

void MainWindow::onRemoveObject()
{
    int row = objectList->currentRow();
    if (row < 0 || (size_t)row >= m_scene.m_objects.size()) return;

    m_scene.removeObject((size_t)row);
    populateObjectList();
    onObjectSelected(-1);
    rebuildScene();
}

void MainWindow::onAddObject()
{
    AddObjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString type = dialog.getSelectedType();
        std::unique_ptr<ActuatorObject> newObject;
        if (type == "Coil") {
            newObject = std::make_unique<Coil>();
        }
        
        if (newObject) {
            m_scene.m_objects.push_back(std::move(newObject));
            populateObjectList();
            objectList->setCurrentRow(objectList->count() - 1);
            rebuildScene(); 
        }
    }
}

void MainWindow::rebuildScene()
{
    if (!openGLWidget->isValid()) return;
    openGLWidget->clearScene();

    if (m_scene.m_objects.empty()) {
        openGLWidget->update();
        return;
    }

    float xOffset = 0.0f;
    const float spacing = 30.0f;
    xOffset -= (m_scene.m_objects.size() - 1) * spacing / 2.0f;

    for (const auto& obj : m_scene.m_objects) {
        TriangleMesh mesh = obj->generateMesh();
        openGLWidget->addObject(mesh, {xOffset, 0, 0});
        xOffset += spacing;
    }

    openGLWidget->update();
}

void MainWindow::updateSimulationMode()
{
    if (m_continuousModeRadio->isChecked()) {
        m_simulation.setSimulationMode(SimulationController::CONTINUOUS);
        m_pulseWidthSpinBox->setEnabled(false);
    } else {
        m_simulation.setSimulationMode(SimulationController::PULSED);
        m_pulseWidthSpinBox->setEnabled(true);
    }
    updateSimulationTime(m_timeSlider->value() / 1000.0f);
}

void MainWindow::onTimeSliderChanged(int value)
{
    updateSimulationTime(value / 1000.0f);
}

void MainWindow::updateSimulationTime(float time)
{
    ActuatorObject* electromagnet = nullptr;
    for(auto& obj : m_scene.m_objects) {
        if (!electromagnet) {
            if (dynamic_cast<Coil*>(obj.get())) {
                electromagnet = obj.get();
            }
        }
    }
    if (!electromagnet) return;

    m_simulation.setObject(electromagnet);
    m_simulation.calculateStateAtTime(time);

    m_timeLabel->setText(QString("Time: %1s").arg(time, 0, 'f', 3));
    m_magneticFieldLabel->setText(QString("B-Field: %1T").arg(m_simulation.magnetic_field, 0, 'f', 3));

    emit calculateFieldLines(electromagnet->getPath(), m_simulation.current);
}

void MainWindow::onFieldLinesCalculated(const std::vector<FieldLine>& fieldLines)
{
    openGLWidget->updateFieldLines(fieldLines);
}

void MainWindow::refreshParameterUI()
{
    if (!m_selectedObject) return;
    auto params = m_selectedObject->getParameters();
    for(size_t i = 0; i < params.size(); ++i) {
        if ((int)i < m_paramEdits.size()) {
            m_paramEdits[i]->setText(QString::number(*params[i].second));
        }
    }
}
