#ifndef SETTINGS_H
#define SETTINGS_H

#include <QMatrix4x4>

class Settings
{
public:
    Settings();

    bool modelLoaded;
    bool wireframeMode;
    int reflectionLinesDensity;
    bool drawReflectionLines;

    int reflectionLineX;
    int reflectionLineY;
    int reflectionLineZ;
    int glPointSize;
    int selectionMode;


    float FoV;
    float dispRatio;
    float rotAngle;

    bool uniformUpdateRequired;

    QMatrix4x4 modelViewMatrix, projectionMatrix;
    QMatrix3x3 normalMatrix;
};

#endif // SETTINGS_H
