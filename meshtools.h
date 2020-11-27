#ifndef MESHTOOLS_H
#define MESHTOOLS_H

#include "mesh.h"
#include <QVector3D>



QVector3D vertexPoint(HalfEdge* firstEdge);
QVector3D edgePoint(HalfEdge* firstEdge);


#endif // MESHTOOLS_H
