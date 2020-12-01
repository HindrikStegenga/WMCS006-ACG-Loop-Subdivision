#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLDebugLogger>

#include <QOpenGLShaderProgram>

#include <QMouseEvent>

#include "meshrenderer.h"
#include "mesh.h"

class MainView : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core {

    Q_OBJECT

public:
    MainView(QWidget *Parent = 0);
    ~MainView();

    void setSubdivisionLevel(int level);
    void updateMatrices();
    void updateUniforms();
    void updateBuffers(Mesh& currentMesh);

protected:
    void initializeGL();
    void resizeGL(int newWidth, int newHeight);
    void paintGL();

    void mouseMoveEvent(QMouseEvent* Event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void keyPressEvent(QKeyEvent* event);

private:
    QOpenGLDebugLogger debugLogger;

    QVector2D toNormalizedScreenCoordinates(int x, int y);

    void createShaderPrograms();
    void createBuffers();

    //for zoom
    float scale;
    //for handling rotation
    QVector3D oldVec;
    QQuaternion rotationQuaternion;
    bool dragging;

    MeshRenderer mr;
    int lastMousePressX, lastMousePressY;

    Settings settings;

    //we make mainwindow a friend so it can access settings
    friend class MainWindow;
private slots:
    void onMessageLogged( QOpenGLDebugMessage Message );

};

#endif // MAINVIEW_H
