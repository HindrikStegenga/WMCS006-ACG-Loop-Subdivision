#include "meshrenderer.h"

// Computes shortest distance to a given line segment
// If outside of line segment gives either lineStart or lineEnd.
float distanceToLineSegment(QVector2D point, QVector2D lineStart, QVector2D lineEnd) {

    float x = point.x();
    float y = point.y();
    float x1 = lineStart.x();
    float y1 = lineStart.y();
    float x2 = lineEnd.x();
    float y2 = lineEnd.y();

    float A = x - x1;
    float B = y - y1;
    float C = x2 - x1;
    float D = y2 - y1;

    float dot = A * C + B * D;
    float len_sq = C * C + D * D;
    float param = -1;
    if (len_sq != 0) //in case of 0 length line
        param = dot / len_sq;

    float xx, yy;

    // This means we are outside of point1, thus p1 is the closest on the linesegment
    if (param < 0) {
        xx = x1;
        yy = y1;
    }
    else if (param > 1) {
        // This means we are outside of poin2, thus p2 is closest point on the linesegment.
        xx = x2;
        yy = y2;
    }
    else {
        xx = x1 + param * C;
        yy = y1 + param * D;
    }

    float dx = x - xx;
    float dy = y - yy;
    return std::sqrt(dx * dx + dy * dy);
}


MeshRenderer::MeshRenderer()
{
    meshIBOSize = 0;
}

MeshRenderer::~MeshRenderer() {
    gl->glDeleteVertexArrays(1, &vao);
    gl->glDeleteVertexArrays(1, &lineSegmentVao);

    gl->glDeleteBuffers(1, &tbo);

    gl->glDeleteBuffers(1, &lineSegmentVBO);
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

    // Set up vertex array for line segment buffer
    gl->glGenVertexArrays(1, &lineSegmentVao);
    gl->glBindVertexArray(lineSegmentVao);

    gl->glGenBuffers(1, &lineSegmentVBO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, lineSegmentVBO);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);
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
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D) * vertexCoords.size(), nullptr, GL_STATIC_READ);
    transformFeedbackBufferSize = sizeof(QVector3D) * vertexCoords.size();

    meshIBOSize = polyIndices.size();
    lastIndexBuffer = polyIndices;
    lastVertexBuffer = vertexCoords;
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
    shaderProg.setUniformValue("selectLine", false);
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
        gl->glDrawArrays(GL_POINTS, 0, transformFeedbackBufferSize / sizeof(QVector3D));
        // Disable transform feedback again.
        gl->glEndTransformFeedback();

        // Wait for completion of drawcommands.
        gl->glFlush();

        // Write back transform feedback buffer to CPU memory.
        transformFeedbackBuffer.resize(transformFeedbackBufferSize / sizeof(QVector3D));
        gl->glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, transformFeedbackBufferSize, transformFeedbackBuffer.data());

        // Compute closest vertex.
        selectedVertex = computeClosestVertex();
        // Cpmpute closest line segment.
        computeClosestLineSegment();

        //Now we redraw everything again!
        // But with enabled rasterization ofcourse
        gl->glDisable(GL_RASTERIZER_DISCARD);
    }

    gl->glDrawElements(GL_TRIANGLES, meshIBOSize, GL_UNSIGNED_INT, 0);

    // Set point update to false.
    pointUpdated = false;

    // Draw point we selected
    if (selectedVertex != -1 && settings->selectionMode == 1) {
        // Set the selectionMode
        shaderProg.setUniformValue("selectionMode", true);
        gl->glPointSize(settings->glPointSize);
        // Draw the point
        gl->glDrawArrays(GL_POINTS, selectedVertex, 1);
    }

    if (selectedVertex != -1 && settings->selectionMode == 2) {
        shaderProg.setUniformValue("selectLine", true);
        gl->glPointSize(settings->glPointSize);

        // Unfortunately, since we have a GL_TRIANGLES indexbuffer, it means we need a separate vao+buffer to draw linesegments in.
        gl->glBindVertexArray(lineSegmentVao);
        gl->glBindBuffer(GL_ARRAY_BUFFER, lineSegmentVBO);
        // Update line segment buffer
        gl->glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D) * 2, (void*)(&lineSegmentBuffer[0]), GL_STATIC_DRAW);

        // Draw the line segment.
        gl->glDrawArrays(GL_LINES, 0, 2);
        // Draw some pretty points
        gl->glPointSize(std::max(1, settings->glPointSize / 2));
        shaderProg.setUniformValue("selectLine", false);
        shaderProg.setUniformValue("selectionMode", true);
        gl->glDrawArrays(GL_POINTS, 0, 2);
        shaderProg.setUniformValue("selectionMode", false);
        shaderProg.setUniformValue("selectLine", false);
    }

    //Reset selection mode
    shaderProg.setUniformValue("selectionMode", false);

    gl->glBindVertexArray(0);

    shaderProg.release();

}


bool isClipped(QVector3D point) {

    return point.x() < -1 || point.x() > 1 ||
            point.y() < -1 || point.y() > 1 ||
            point.z() < -1 || point.z() > 1;
}


// Computes closest line segment.
// It is implemented as the smalled NDC space distance to the midpoint of a edge.
void MeshRenderer::computeClosestLineSegment() {
    if (!pointUpdated) {
        return;
    }
    // Initial distance is max float
    float distanceFromLine = 3.402823E+38;
    bool isSet = false;

    QVector3D firstVertex;
    QVector3D secondVertex;

    // Iterate over index buffer in sets of 3, since we use GL_TRIANGLES layout for the index buffer.
    for(int i = 0; i < lastIndexBuffer.size(); i += 3) {
        // Grab the vertices of our triangle in NDC space
        auto v1 = transformFeedbackBuffer[lastIndexBuffer[i]];
        auto v2 = transformFeedbackBuffer[lastIndexBuffer[i + 1]];
        auto v3 = transformFeedbackBuffer[lastIndexBuffer[i + 2]];

        // Compute distances to line segments.
        float d1 = distanceToLineSegment(lastPickedPoint, QVector2D(v1), QVector2D(v2));
        float d2 = distanceToLineSegment(lastPickedPoint, QVector2D(v2), QVector2D(v3));
        float d3 = distanceToLineSegment(lastPickedPoint, QVector2D(v3), QVector2D(v1));

        // Set shortest distance and store it
        // We also need to check for clipping, since we don't want invisible vertices to affect our selection.
        // I also chose to only allow fully visible edges. (i.e. both vertices must be within the visible space).
        if (i == 0 || ((d1 < distanceFromLine) && !(isClipped(v1) || isClipped(v2)))) {
            distanceFromLine = d1;
            firstVertex = lastVertexBuffer[lastIndexBuffer[i]];
            secondVertex = lastVertexBuffer[lastIndexBuffer[i + 1]];

            if ((d1 < distanceFromLine) && !(isClipped(v1) || isClipped(v2)))
                isSet = true;
        }
        if (d2 < distanceFromLine && !(isClipped(v2) || isClipped(v3))) {
            distanceFromLine = d2;
            firstVertex = lastVertexBuffer[lastIndexBuffer[i + 1]];
            secondVertex = lastVertexBuffer[lastIndexBuffer[i + 2]];
            isSet = true;
        }
        if (d3 < distanceFromLine && !(isClipped(v3) || isClipped(v1))) {
            distanceFromLine = d3;
            firstVertex = lastVertexBuffer[lastIndexBuffer[i + 2]];
            secondVertex = lastVertexBuffer[lastIndexBuffer[i]];
            isSet = true;
        }
    }

    // These two points together form the closest line segment.
    if (isSet) {
        lineSegmentBuffer[0] = firstVertex;
        lineSegmentBuffer[1] = secondVertex;
    } else {
        qDebug() << "Weirdness occured!";
    }
}

// Computes the vertex index which is closest to the lastPickedPointlastPickedPoint.
int MeshRenderer::computeClosestVertex() {
    if (!pointUpdated) {
        return -1;
    }

    // Initial distance is max float
    float distanceFromNDCSelectedPoint = 3.402823E+38;
    int closestIndex = 0;
    // Effectively computes closest vertex in NDC space to the picket point (also in NDC space).
    for(int i = 0; i < transformFeedbackBuffer.size(); ++i) {
        if (i == 0) {
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
    return closestIndex;
}
