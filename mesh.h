#ifndef MESH_H
#define MESH_H

#include <QVector>

#include "vertex.h"
#include "face.h"
#include "halfedge.h"

#include "objfile.h"

class Mesh {

public:
    Mesh();
    Mesh(OBJFile *loadedOBJFile);
    ~Mesh();

    inline QVector<Vertex>& getVertices() { return vertices; }
    inline QVector<HalfEdge>& getHalfEdges() { return halfEdges; }
    inline QVector<Face>& getFaces() { return faces; }

    inline QVector<QVector3D>& getVertexCoords() { return vertexCoords; }
    inline QVector<QVector3D>& getVertexNorms() { return vertexNormals; }
    inline QVector<unsigned int>& getPolyIndices() { return polyIndices; }

    void extractAttributes();

    void setTwins(unsigned int numHalfEdges, unsigned int indexH, QVector<QVector<unsigned int>>& potentialTwins);
    void setFaceNormal(Face& currentFace);
    QVector3D computeVertexNormal(Vertex& currentVertex);

    // For debugging
    void dispVertInfo(Vertex& dVert);
    void dispHalfEdgeInfo(HalfEdge& dHalfEdge);
    void dispFaceInfo(Face& dFace);

    void subdivideLoop(Mesh& mesh);
    void splitHalfEdges(QVector<Vertex>& newVertices, QVector<HalfEdge>& newHalfEdges);

private:
    QVector<QVector3D> vertexCoords;
    QVector<QVector3D> vertexNormals;
    QVector<unsigned int> polyIndices;

    QVector<Vertex> vertices;
    QVector<Face> faces;
    QVector<HalfEdge> halfEdges;
};

#endif // MESH_H
