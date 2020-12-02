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
    void computeClosestLineSegment();

    QVector<unsigned int> lastIndexBuffer;
    QVector<QVector3D> lastVertexBuffer;

    QVector3D lineSegmentBuffer[2];
    QVector<QVector3D> transformFeedbackBuffer;
    QVector2D lastPickedPoint;
    bool pointUpdated = false;
    int selectedVertex = -1;
private:

    int transformFeedbackBufferSize;
    GLuint vao, lineSegmentVao;
    GLuint tbo; //Transform feedback buffer
    GLuint meshCoordsBO, meshNormalsBO, meshIndexBO, lineSegmentVBO;
    unsigned int meshIBOSize;
    QOpenGLShaderProgram shaderProg;

    // Uniforms
    GLint uniModelViewMatrix, uniProjectionMatrix, uniNormalMatrix;
};

#endif // MESHRENDERER_H
