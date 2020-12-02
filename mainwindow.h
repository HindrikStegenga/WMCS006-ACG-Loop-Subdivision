#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "objfile.h"
#include <QFileDialog>
#include "mesh.h"
#include "meshtools.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {

    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void loadOBJ();
    QVector<Mesh> meshes;

private slots:
    void on_RotateDial_valueChanged(int value);
    void on_SubdivSteps_valueChanged(int value);
    void on_glPointSize_valueChanged(int value);
    void on_drawReflectionLines_toggled(bool checked);
    void on_selectionMode_currentIndexChanged(int index);
    void on_reflectionLinesDensity_valueChanged(int value);

    void on_reflectionLinesNormalX_valueChanged(int value);
    void on_reflectionLinesNormalY_valueChanged(int value);
    void on_reflectionLinesNormalZ_valueChanged(int value);

    void on_LoadOBJ_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
