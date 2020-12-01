#ifndef MESHRENDERER_H
#define MESHRENDERER_H

#include <QOpenGLShaderProgram>

#include "renderer.h"
#include "mesh.h"
#include <memory>

using std::unique_ptr;

class MeshRenderer : public Renderer
{
public:
    MeshRenderer();
    ~MeshRenderer();

    void init(QOpenGLFunctions_4_1_Core* f, Settings* s);

    void initShaders();
    void initBuffers();

    void updateUniforms();

    void updateBuffers(Mesh& m);
    void draw();

    int computeClosestVertex();

    QVector<QVector2D> transformFeedbackBuffer;
    QVector2D lastPickedPoint;
    bool pointUpdated = false;
private:
    int selectedVertex = -1;
    int transformFeedbackBufferSize;
    GLuint vao;
    GLuint tbo; //Transform feedback buffer
    GLuint meshCoordsBO, meshNormalsBO, meshIndexBO;
    unsigned int meshIBOSize;
    QOpenGLShaderProgram shaderProg;

    // Uniforms
    GLint uniModelViewMatrix, uniProjectionMatrix, uniNormalMatrix;
};

#endif // MESHRENDERER_H
