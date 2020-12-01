#include "meshrenderer.h"

MeshRenderer::MeshRenderer()
{
    meshIBOSize = 0;
}

MeshRenderer::~MeshRenderer() {
    gl->glDeleteVertexArrays(1, &vao);

    gl->glDeleteBuffers(1, &tbo);

    gl->glDeleteBuffers(1, &meshCoordsBO);
    gl->glDeleteBuffers(1, &meshNormalsBO);
    gl->glDeleteBuffers(1, &meshIndexBO);
}

void MeshRenderer::init(QOpenGLFunctions_4_1_Core* f, Settings* s) {
    gl = f;
    settings = s;

    initShaders();
    initBuffers();
}

void MeshRenderer::initShaders() {

    shaderProg.create();
    shaderProg.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertshader.glsl");
    shaderProg.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragshader.glsl");

    const GLchar* feedbackVaryings[] = { "vertcoords_ndc" };
    gl->glTransformFeedbackVaryings(shaderProg.programId(), 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);

    shaderProg.link();

    uniModelViewMatrix = gl->glGetUniformLocation(shaderProg.programId(), "modelviewmatrix");
    uniProjectionMatrix = gl->glGetUniformLocation(shaderProg.programId(), "projectionmatrix");
    uniNormalMatrix = gl->glGetUniformLocation(shaderProg.programId(), "normalmatrix");
}

void MeshRenderer::initBuffers() {

    gl->glGenVertexArrays(1, &vao);
    gl->glBindVertexArray(vao);

    gl->glGenBuffers(1, &meshCoordsBO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, meshCoordsBO);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    gl->glGenBuffers(1, &meshNormalsBO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, meshNormalsBO);
    gl->glEnableVertexAttribArray(1);
    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    gl->glGenBuffers(1, &meshIndexBO);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO);

    gl->glGenBuffers(1, &tbo);
    gl->glBindBuffer(GL_ARRAY_BUFFER, tbo);

    gl->glBindVertexArray(0);
}

void MeshRenderer::updateBuffers(Mesh& m) {
    //gather attributes for current mesh
    m.extractAttributes();
    QVector<QVector3D>& vertexCoords = m.getVertexCoords();
    QVector<QVector3D>& vertexNormals = m.getVertexNorms();
    QVector<unsigned int>& polyIndices = m.getPolyIndices();

    gl->glBindBuffer(GL_ARRAY_BUFFER, meshCoordsBO);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*vertexCoords.size(), vertexCoords.data(), GL_STATIC_DRAW);

    gl->glBindBuffer(GL_ARRAY_BUFFER, meshNormalsBO);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*vertexNormals.size(), vertexNormals.data(), GL_STATIC_DRAW);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*polyIndices.size(), polyIndices.data(), GL_STATIC_DRAW);

    // Bind transform feedback buffer.
    gl->glBindBuffer(GL_ARRAY_BUFFER, tbo);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(QVector2D) * vertexCoords.size(), nullptr, GL_STATIC_READ);
    transformFeedbackBufferSize = sizeof(QVector2D) * vertexCoords.size();

    meshIBOSize = polyIndices.size();
}

void MeshRenderer::updateUniforms() {
    gl->glUniformMatrix4fv(uniModelViewMatrix, 1, false, settings->modelViewMatrix.data());
    gl->glUniformMatrix4fv(uniProjectionMatrix, 1, false, settings->projectionMatrix.data());
    gl->glUniformMatrix3fv(uniNormalMatrix, 1, false, settings->normalMatrix.data());
}

void MeshRenderer::draw() {
    gl->glClearColor(0.0, 0.0, 0.0, 1.0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shaderProg.bind();

    if (settings->uniformUpdateRequired) {
        updateUniforms();
        settings->uniformUpdateRequired = false;
    }

    // Reset selection mode and update all other uniforms.
    shaderProg.setUniformValue("selectionMode", false);
    shaderProg.setUniformValue("drawReflectionLines", settings->drawReflectionLines);
    shaderProg.setUniformValue("sineScale", (float)settings->reflectionLinesDensity);
    shaderProg.setUniformValue("testNormal", (float)settings->reflectionLineX, (float)settings->reflectionLineY, (float)settings->reflectionLineZ);


    gl->glBindVertexArray(vao);

    // Okay so this is kinda cheesy/hacky, but I use the transform feedback buffer to grab all coordinates of the vertices in NDC space.
    // This is done by binding the vertex buffer as GL_POINTS so I get vertices in my feedback buffer.
    // First I disabled rasterization output for rendering my vertices, since we don't want output on the screen.
    // I modified the shader such that it outputs NDC coordinates for every vertex.
    // These coordinates are ignored under normal rendering, but they are outputed to the transform feedback buffer
    // on a per point/vertex basis. So therefore, the gpu first calculates NDC coordinates for all my vertices.
    // After which I simply copy it back to host memory (RAM), and compute the closest point with the NDC
    // picking point I got from the mouse on the cpu.
    // After which I enable rasterization again and redraw everything as I normally would.
    // Yes, yes... this approach is really stupid and horribly inneficient... but it's fun to implement and it works!
    // It avoids me from doing complicated maths in order to revert the projections. =)
    // After determining the closest vertex, I simply redraw my vertices as points and make a big nice dot at the vertex position.

    if (pointUpdated) {
        // Bind transform feedback buffer.
        gl->glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);
        // Discard geometry at rasterization, since we only want transform feedback output.
        gl->glEnable(GL_RASTERIZER_DISCARD);
        // Enable transform feedback
        gl->glBeginTransformFeedback(GL_POINTS);
        // Draw into transform feedback
        gl->glDrawArrays(GL_POINTS, 0, transformFeedbackBufferSize / sizeof(QVector2D));
        // Disable transform feedback again.
        gl->glEndTransformFeedback();

        // Wait for completion of drawcommands.
        gl->glFlush();

        // Write back transform feedback buffer to CPU memory.
        transformFeedbackBuffer.resize(transformFeedbackBufferSize / sizeof(QVector2D));
        gl->glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, transformFeedbackBufferSize, transformFeedbackBuffer.data());

        // Compute closest vertex.
        selectedVertex = computeClosestVertex();

        //Now we redraw everything again!
        // But with enabled rasterization ofcourse
        gl->glDisable(GL_RASTERIZER_DISCARD);
    }

    gl->glDrawElements(GL_TRIANGLES, meshIBOSize, GL_UNSIGNED_INT, 0);

    // Set point update to false.
    pointUpdated = false;

    // Draw point we selected
    if (selectedVertex != -1) {
        // Set the selectionMode
        shaderProg.setUniformValue("selectionMode", true);
        gl->glPointSize(settings->glPointSize);
        // Draw the point
        gl->glDrawArrays(GL_POINTS, selectedVertex, 1);
    }
    //Reset selection mode
    shaderProg.setUniformValue("selectionMode", false);

    gl->glBindVertexArray(0);

    shaderProg.release();

}

int MeshRenderer::computeClosestVertex() {
    if (!pointUpdated) {
        return -1;
    }

    float distanceFromNDCSelectedPoint = -1.0;
    int closestIndex = 0;
    for(int i = 0; i < transformFeedbackBuffer.size(); ++i) {
        if (distanceFromNDCSelectedPoint == -1.0) {
            distanceFromNDCSelectedPoint = abs(transformFeedbackBuffer[i].distanceToPoint(lastPickedPoint));
            closestIndex = i;
        } else {
            float distance = abs(transformFeedbackBuffer[i].distanceToPoint(lastPickedPoint));
            if (distance < distanceFromNDCSelectedPoint) {
                distanceFromNDCSelectedPoint = distance;
                closestIndex = i;
            }
        }
    }
    qDebug() << "selected: " << closestIndex;
    return closestIndex;
}
