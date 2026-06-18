#include "openglwidget.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <vector>

// Shader now includes a model matrix for object positioning and vertex color support
static const char *vertexShaderSource =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec3 aNormal;\n"
    "layout(location = 2) in vec3 aColor;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "out vec3 Color;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform bool useVertexColor;\n"
    "uniform vec3 objectColor;\n"
    "void main()\n"
    "{\n"
    "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
    "   Color = useVertexColor ? aColor : objectColor;\n"
    "   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
    "}\n";

// Simple diffuse lighting shader
static const char *fragmentShaderSource =
    "#version 330 core\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "in vec3 Color;\n"
    "out vec4 FragColor;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "void main()\n"
    "{\n"
    "   // Ambient\n"
    "   float ambientStrength = 0.2;\n"
    "   vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);\n"
    "   // Diffuse\n"
    "   vec3 norm = normalize(Normal);\n"
    "   vec3 lightDir = normalize(lightPos - FragPos);\n"
    "   float diff = max(dot(norm, lightDir), 0.0);\n"
    "   vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);\n"
    "   vec3 result = (ambient + diffuse) * Color;\n"
    "   FragColor = vec4(result, 1.0);\n"
    "}\n";

OpenGLWidget::OpenGLWidget(QWidget *parent) : QOpenGLWidget(parent) {}

OpenGLWidget::~OpenGLWidget()
{
    makeCurrent();
    clearScene();
    if (m_gridObject) {
        m_gridObject->vao.destroy();
        m_gridObject->vbo.destroy();
        m_gridObject->ebo.destroy();
        m_gridObject.reset();
    }
    if (m_fieldLinesObject) {
        m_fieldLinesObject->vao.destroy();
        m_fieldLinesObject->vbo.destroy();
        m_fieldLinesObject->ebo.destroy();
        m_fieldLinesObject.reset();
    }
    delete m_program;
    doneCurrent();
}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();
    
    setupScene(); // For grid/axes
    updateCamera();
}

void OpenGLWidget::paintGL()
{
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f); // Dark grey background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_program->bind();

    // Pass projection and view matrices to the shader
    m_program->setUniformValue("projection", QMatrix4x4(m_projection.constData()).transposed());
    m_program->setUniformValue("view", QMatrix4x4(m_view.constData()).transposed());

    // Pass lighting and camera position info
    float yawRad = m_cameraYaw * (3.14159f / 180.0f);
    float pitchRad = m_cameraPitch * (3.14159f / 180.0f);
    Vec3 eye;
    eye.x = m_cameraTarget.x + m_cameraDistance * cos(pitchRad) * sin(yawRad);
    eye.y = m_cameraTarget.y + m_cameraDistance * sin(pitchRad);
    eye.z = m_cameraTarget.z + m_cameraDistance * cos(pitchRad) * cos(yawRad);
    m_program->setUniformValue("lightPos", QVector3D(eye.x, eye.y, eye.z));
    m_program->setUniformValue("viewPos", QVector3D(eye.x, eye.y, eye.z));

    // Draw Grid
    if (m_gridObject) {
        m_program->setUniformValue("model", QMatrix4x4(m_gridObject->model_matrix.constData()).transposed());
        m_program->setUniformValue("objectColor", QVector3D(0.4f, 0.4f, 0.4f));
        m_program->setUniformValue("useVertexColor", false);
        m_gridObject->vao.bind();
        glDrawElements(m_gridObject->drawMode, m_gridObject->index_count, GL_UNSIGNED_INT, 0);
        m_gridObject->vao.release();
    }

    // Draw Field Lines
    if (m_fieldLinesObject) {
        m_program->setUniformValue("model", QMatrix4x4(m_fieldLinesObject->model_matrix.constData()).transposed());
        m_program->setUniformValue("useVertexColor", true);
        m_fieldLinesObject->vao.bind();
        glDrawElements(m_fieldLinesObject->drawMode, m_fieldLinesObject->index_count, GL_UNSIGNED_INT, 0);
        m_fieldLinesObject->vao.release();
    }

    // Render all objects
    for (const auto& objPtr : m_renderObjects) {
        RenderObject* obj = objPtr.get();
        m_program->setUniformValue("model", QMatrix4x4(obj->model_matrix.constData()).transposed());
        m_program->setUniformValue("objectColor", QVector3D(0.8f, 0.7f, 0.6f));
        m_program->setUniformValue("useVertexColor", false);

        obj->vao.bind();
        glDrawElements(obj->drawMode, obj->index_count, GL_UNSIGNED_INT, 0);
        obj->vao.release();
    }
    m_program->release();
}

void OpenGLWidget::resizeGL(int w, int h)
{
    float aspect = float(w) / float(h ? h : 1);
    const float fov = 45.0f * (3.14159f / 180.0f);
    m_projection = Mat4::createPerspective(fov, aspect, 0.1f, 1000.0f);
}

void OpenGLWidget::setObjectPosition(int index, const Vec3& position) {
    if (index >= 0 && index < (int)m_renderObjects.size()) {
        m_renderObjects[index]->model_matrix = Mat4::createTranslation(position);
        update();
    }
}

void OpenGLWidget::updateFieldLines(const std::vector<FieldLine>& lines) {
    makeCurrent();
    if (m_fieldLinesObject) {
        m_fieldLinesObject->vao.destroy();
        m_fieldLinesObject->vbo.destroy();
        m_fieldLinesObject->ebo.destroy();
        m_fieldLinesObject.reset();
    }
    
    if (lines.empty()) {
        doneCurrent();
        update();
        return;
    }
    
    m_fieldLinesObject = std::make_unique<RenderObject>();
    m_fieldLinesObject->drawMode = GL_LINES;
    m_fieldLinesObject->model_matrix = Mat4::identity();
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Find min/max intensity for scaling
    std::vector<float> all_b;
    for(const auto& line : lines) {
        for(float b : line.intensity) {
            all_b.push_back(b);
        }
    }
    
    float min_b = 0.0f;
    float max_b = 1.0f;
    
    if (!all_b.empty()) {
        std::sort(all_b.begin(), all_b.end());
        min_b = all_b.front();
        // Use 90th percentile to ignore singularities near the wire
        max_b = all_b[size_t(all_b.size() * 0.90)];
    }
    
    if (max_b <= min_b) max_b = min_b + 1e-6f; // Avoid divide by zero
    
    qInfo() << "Field Simulation Result: Max Intensity (90th percentile) =" << max_b << "Tesla";

    unsigned int idx = 0;
    for (const auto& line : lines) {
        if (line.points.size() < 2) continue;
        for (size_t i = 0; i < line.points.size() - 1; ++i) {
            Vec3 p1 = line.points[i];
            Vec3 p2 = line.points[i+1];
            float b1 = line.intensity[i];
            float b2 = line.intensity[i+1];
            
            // Map B to Color (Blue -> Red)
            float t1 = (b1 - min_b) / (max_b - min_b);
            float t2 = (b2 - min_b) / (max_b - min_b);
            
            // Clamp t1 and t2 in case b is above our 90th percentile max_b
            if (t1 > 1.0f) t1 = 1.0f;
            if (t1 < 0.0f) t1 = 0.0f;
            if (t2 > 1.0f) t2 = 1.0f;
            if (t2 < 0.0f) t2 = 0.0f;
            // Simple Blue(0,0,1) to Red(1,0,0) mix? Or heatmap.
            // Let's do: Low=Blue, Mid=Green, High=Red.
            auto getColor = [](float t) -> Vec3 {
                float r=0, g=0, b=0;
                if (t < 0.5f) {
                    // Blue to Green
                    float tt = t * 2.0f;
                    b = 1.0f - tt;
                    g = tt;
                } else {
                    // Green to Red
                    float tt = (t - 0.5f) * 2.0f;
                    g = 1.0f - tt;
                    r = tt;
                }
                return {r, g, b};
            };
            
            Vec3 c1 = getColor(t1);
            Vec3 c2 = getColor(t2);
            
            // P1: Pos(3), Norm(3), Color(3)
            vertices.push_back(p1.x); vertices.push_back(p1.y); vertices.push_back(p1.z);
            vertices.push_back(0); vertices.push_back(1); vertices.push_back(0); 
            vertices.push_back(c1.x); vertices.push_back(c1.y); vertices.push_back(c1.z);
            
            // P2
            vertices.push_back(p2.x); vertices.push_back(p2.y); vertices.push_back(p2.z);
            vertices.push_back(0); vertices.push_back(1); vertices.push_back(0);
            vertices.push_back(c2.x); vertices.push_back(c2.y); vertices.push_back(c2.z);
            
            indices.push_back(idx++);
            indices.push_back(idx++);
        }
    }
    
    m_fieldLinesObject->index_count = indices.size();
    
    m_fieldLinesObject->vao.create();
    m_fieldLinesObject->vao.bind();
    
    m_fieldLinesObject->vbo.create();
    m_fieldLinesObject->vbo.bind();
    m_fieldLinesObject->vbo.allocate(vertices.data(), vertices.size() * sizeof(float));
    
    m_fieldLinesObject->ebo.create();
    m_fieldLinesObject->ebo.bind();
    m_fieldLinesObject->ebo.allocate(indices.data(), indices.size() * sizeof(unsigned int));
    
    // Stride is 9 floats now
    int stride = 9 * sizeof(float);
    
    // Pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    // Norm
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Color
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    m_fieldLinesObject->vao.release();
    doneCurrent();
    update();
}

void OpenGLWidget::addObject(const TriangleMesh& mesh, const Vec3& position)
{
    makeCurrent();
    std::unique_ptr<RenderObject> obj = std::make_unique<RenderObject>();
    obj->model_matrix = Mat4::createTranslation(position);
    obj->index_count = mesh.indices.size();

    obj->vao.create();
    obj->vao.bind();

    // Create a single VBO for vertices and normals interleaved
    std::vector<float> interleaved_data;
    for(size_t i = 0; i < mesh.vertices.size(); ++i) {
        interleaved_data.push_back(mesh.vertices[i].x);
        interleaved_data.push_back(mesh.vertices[i].y);
        interleaved_data.push_back(mesh.vertices[i].z);
        if (i < mesh.normals.size()) {
            interleaved_data.push_back(mesh.normals[i].x);
            interleaved_data.push_back(mesh.normals[i].y);
            interleaved_data.push_back(mesh.normals[i].z);
        } else { // Add dummy normal if none provided
            interleaved_data.push_back(0);
            interleaved_data.push_back(1);
            interleaved_data.push_back(0);
        }
    }
    
    obj->vbo.create();
    obj->vbo.bind();
    obj->vbo.allocate(interleaved_data.data(), interleaved_data.size() * sizeof(float));

    obj->ebo.create();
    obj->ebo.bind();
    obj->ebo.allocate(mesh.indices.data(), mesh.indices.size() * sizeof(unsigned int));
    
    // Vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Vertex normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    obj->vao.release();
    doneCurrent();

    m_renderObjects.push_back(std::move(obj));
}

void OpenGLWidget::clearScene()
{
    makeCurrent();
    for (auto& obj : m_renderObjects) {
        obj->vao.destroy();
        obj->vbo.destroy();
        obj->ebo.destroy();
    }
    m_renderObjects.clear();
    doneCurrent();
}

void OpenGLWidget::setupScene()
{
    // Grid generation
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    int size = 20; // Number of lines on each side of center
    float step = 10.0f;
    
    // X lines
    for(int i = -size; i <= size; ++i) {
        float z = i * step;
        // P1
        vertices.push_back(-size * step); vertices.push_back(0); vertices.push_back(z);
        vertices.push_back(0); vertices.push_back(1); vertices.push_back(0); // Normal
        // P2
        vertices.push_back(size * step); vertices.push_back(0); vertices.push_back(z);
        vertices.push_back(0); vertices.push_back(1); vertices.push_back(0); // Normal
    }
    // Z lines
    for(int i = -size; i <= size; ++i) {
        float x = i * step;
        // P1
        vertices.push_back(x); vertices.push_back(0); vertices.push_back(-size * step);
        vertices.push_back(0); vertices.push_back(1); vertices.push_back(0); // Normal
        // P2
        vertices.push_back(x); vertices.push_back(0); vertices.push_back(size * step);
        vertices.push_back(0); vertices.push_back(1); vertices.push_back(0); // Normal
    }
    
    for(size_t i = 0; i < vertices.size() / 6; ++i) {
        indices.push_back(i);
    }
    
    makeCurrent();
    if (m_gridObject) {
        m_gridObject->vao.destroy();
        m_gridObject->vbo.destroy();
        m_gridObject->ebo.destroy();
    }
    m_gridObject = std::make_unique<RenderObject>();
    m_gridObject->drawMode = GL_LINES;
    m_gridObject->index_count = indices.size();
    m_gridObject->model_matrix = Mat4::identity();
    
    m_gridObject->vao.create();
    m_gridObject->vao.bind();
    
    m_gridObject->vbo.create();
    m_gridObject->vbo.bind();
    m_gridObject->vbo.allocate(vertices.data(), vertices.size() * sizeof(float));
    
    m_gridObject->ebo.create();
    m_gridObject->ebo.bind();
    m_gridObject->ebo.allocate(indices.data(), indices.size() * sizeof(unsigned int));
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    m_gridObject->vao.release();
    doneCurrent();
}


void OpenGLWidget::updateCamera()
{
    float yawRad = m_cameraYaw * (3.14159f / 180.0f);
    float pitchRad = m_cameraPitch * (3.14159f / 180.0f);

    Vec3 eye;
    eye.x = m_cameraTarget.x + m_cameraDistance * cos(pitchRad) * sin(yawRad);
    eye.y = m_cameraTarget.y + m_cameraDistance * sin(pitchRad);
    eye.z = m_cameraTarget.z + m_cameraDistance * cos(pitchRad) * cos(yawRad);

    m_view = Mat4::createLookAt(eye, m_cameraTarget, {0.0f, 1.0f, 0.0f});
    update();
}

void OpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->position();
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    float dx = event->position().x() - m_lastMousePos.x();
    float dy = event->position().y() - m_lastMousePos.y();

    if (event->buttons() & Qt::LeftButton) {
        m_cameraYaw += dx * 0.5f;
        m_cameraPitch -= dy * 0.5f;
        if(m_cameraPitch > 89.0f) m_cameraPitch = 89.0f;
        if(m_cameraPitch < -89.0f) m_cameraPitch = -89.0f;
    } else if (event->buttons() & Qt::MiddleButton) {
        // Pan logic needs view matrix to get right/up vectors
        Mat4 invView = m_view; // Simplified inverse for ortho camera
        // For a proper perspective camera, a full matrix inverse is needed.
        // This is a simplification.
        Vec3 right = {invView.m[0][0], invView.m[1][0], invView.m[2][0]};
        Vec3 up = {invView.m[0][1], invView.m[1][1], invView.m[2][1]};
        
        m_cameraTarget = m_cameraTarget - right * (dx * 0.1f);
        m_cameraTarget = m_cameraTarget + up * (dy * 0.1f);
    }
    m_lastMousePos = event->position();
    updateCamera();
}

void OpenGLWidget::wheelEvent(QWheelEvent *event)
{
    m_cameraDistance -= event->angleDelta().y() / 120.0f;
    if (m_cameraDistance < 1.0f) {
        m_cameraDistance = 1.0f;
    }
    updateCamera();
}
