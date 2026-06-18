#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <vector>
#include <memory>
#include "vec3.h"
#include "mat4.h"
#include "geometry.h"

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget();

    void addObject(const TriangleMesh& mesh, const Vec3& position = {0,0,0});
    void setObjectPosition(int index, const Vec3& position);
    void updateFieldLines(const std::vector<FieldLine>& lines);
    void clearScene();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

signals:
    void initialized();

private:
    struct RenderObject {
        QOpenGLVertexArrayObject vao;
        QOpenGLBuffer vbo;
        QOpenGLBuffer ebo;
        int index_count;
        Mat4 model_matrix;
        GLenum drawMode = GL_TRIANGLES;

        RenderObject() : ebo(QOpenGLBuffer::IndexBuffer), index_count(0) {}
    };
    
    void setupScene(); // For grid and axes
    void updateCamera();

    QOpenGLShaderProgram *m_program = nullptr;
    std::vector<std::unique_ptr<RenderObject>> m_renderObjects;
    std::unique_ptr<RenderObject> m_gridObject;
    std::unique_ptr<RenderObject> m_fieldLinesObject;

    Mat4 m_projection;
    Mat4 m_view;

    // Camera state
    float m_cameraYaw = -45.0f;
    float m_cameraPitch = 30.0f;
    float m_cameraDistance = 50.0f; // Increased distance to see all objects
    Vec3 m_cameraTarget = {0.0f, 0.0f, 0.0f};

    // Mouse state
    QPointF m_lastMousePos; // Changed to QPointF
};

#endif // OPENGLWIDGET_H
