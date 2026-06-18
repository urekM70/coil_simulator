QT       += core gui widgets opengl openglwidgets

CONFIG += c++20
# Per constraints, language is C++20

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    openglwidget.cpp \
    mesh_generator.cpp \
    scene.cpp \
    ngspice_interface.cpp \
    simulation_controller.cpp \
    simulation_worker.cpp

HEADERS += \
    mainwindow.h \
    openglwidget.h \
    vec3.h \
    mat4.h \
    geometry.h \
    actuator_objects.h \
    mesh_generator.h \
    scene.h \
    addobjectdialog.h \
    ngspice_interface.h \
    simulation_controller.h \
    simulation_worker.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

