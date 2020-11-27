#include "meshtools.h"

void Mesh::subdivideLoop(Mesh& mesh) {
    QVector<Vertex>& newVertices = mesh.getVertices();
    QVector<HalfEdge>& newHalfEdges = mesh.getHalfEdges();
    QVector<Face>& newFaces = mesh.getFaces();

    unsigned int numVerts, numHalfEdges, numFaces;
    unsigned int vIndex, hIndex, fIndex;
    HalfEdge* currentEdge;

    qDebug() << ":: Creating new Loop mesh";

    numVerts = vertices.size();
    numHalfEdges = halfEdges.size();
    numFaces = faces.size();

    // Reserve memory
    newVertices.reserve(numVerts + numHalfEdges / 2);
    newHalfEdges.reserve(2*numHalfEdges + 6*numFaces);
    newFaces.reserve(4*numFaces);

    // Create vertex points
    for (unsigned int k = 0; k < numVerts; k++) {
        // Coords (x,y,z), Out, Valence, Index
        newVertices.push_back( Vertex( vertexPoint(vertices[k].out),
                                             nullptr,
                                             vertices[k].val,
                                             k) );
    }

    vIndex = numVerts;
    qDebug() << " * Created vertex points";

    // Create edge points
    for (unsigned int k = 0; k < numHalfEdges; k++) {
        currentEdge = &halfEdges[k];

        //only create a new vertex per set of halfEdges
        if (k < currentEdge->twin->index) {
            // Coords (x,y,z), Out, Valence, Index
            newVertices.push_back( Vertex(edgePoint(currentEdge),
                                                nullptr,
                                                6,
                                                vIndex) );
            vIndex++;
        }
    }

    qDebug() << " * Created edge points";

    // Split halfedges
    splitHalfEdges(newVertices, newHalfEdges);

    qDebug() << " * Split halfedges";

    hIndex = 2*numHalfEdges;
    fIndex = 0;

    // Create faces and remaining halfedges
    for (unsigned int k = 0; k < numFaces; k++) {
        currentEdge = faces[k].side;

        // Three outer faces

        for (unsigned int m = 0; m < 3; m++) {

            unsigned int s = currentEdge->prev->index;
            unsigned int t = currentEdge->index;

            // Side, Val, Index
            newFaces.push_back( Face(nullptr,
                                3,
                                fIndex) );

            newFaces[fIndex].side = &newHalfEdges[ 2*t ];

            // Target, Next, Prev, Twin, Poly, Index
            newHalfEdges.append(HalfEdge( newHalfEdges[2*s].target,
                                         &newHalfEdges[2*s+1],
                    &newHalfEdges[ 2*t ],
                    nullptr,
                    &newFaces[fIndex],
                    hIndex ));

            newHalfEdges.append(HalfEdge( newHalfEdges[2*t].target,
                                         nullptr,
                                         nullptr,
                                         &newHalfEdges[hIndex],
                                         nullptr,
                                         hIndex+1 ));

            newHalfEdges[hIndex].twin = &newHalfEdges[hIndex+1];

            newHalfEdges[2*s+1].next = &newHalfEdges[2*t];
            newHalfEdges[2*s+1].prev = &newHalfEdges[hIndex];
            newHalfEdges[2*s+1].polygon = &newFaces[fIndex];

            newHalfEdges[2*t].next = &newHalfEdges[hIndex];
            newHalfEdges[2*t].prev = &newHalfEdges[2*s+1];
            newHalfEdges[2*t].polygon = &newFaces[fIndex];

            // For edge points
            newHalfEdges[2*t].target->out = &newHalfEdges[hIndex];

            hIndex += 2;
            fIndex++;
            currentEdge = currentEdge->next;

        }

        // Inner face
        // Side, Val, Index
        newFaces.append(Face(&newHalfEdges[ hIndex-1 ], 3, fIndex));

        for (unsigned int m = 0; m < 3; m++) {

            if (m == 2) {
                newHalfEdges[hIndex - 1].next = &newHalfEdges[hIndex - 5];
            } else {
                newHalfEdges[hIndex - 5 + 2*m].next = &newHalfEdges[hIndex - 5 + 2*(m+1)];
            }

            if (m == 0) {
                newHalfEdges[hIndex - 5].prev = &newHalfEdges[hIndex - 1];
            } else {
                newHalfEdges[hIndex - 5 + 2*m].prev = &newHalfEdges[hIndex - 5 + 2*(m-1)];
            }

            newHalfEdges[hIndex - 5 + 2*m].polygon = &newFaces[fIndex];

        }

        fIndex++;

    }

    qDebug() << " * Created faces";



    // set outs for updated vertices
    for (unsigned int k = 0; k < numVerts; k++) {
        newVertices[k].out = &newHalfEdges[ 2 * vertices[k].out->index ];
    }
}

// ---

QVector3D vertexPoint(HalfEdge* firstEdge) {
    unsigned short k, n;
    QVector3D sumStarPts, sumFacePts;
    QVector3D vertexPt;
    float stencilValue;
    HalfEdge* currentEdge;
    Vertex* currentVertex;

    currentVertex = firstEdge->twin->target;
    n = currentVertex->val;

    sumStarPts = QVector3D();
    sumFacePts = QVector3D();
    currentEdge = firstEdge;

    for (k=0; k<n; k++) {
        sumStarPts += currentEdge->target->coords;
        currentEdge = currentEdge->prev->twin;
    }

    // Warren's rules
    if (n == 3) {
        stencilValue = 3.0/16.0;
    }
    else {
        stencilValue = 3.0/(8*n);
    }

    vertexPt = (1.0 - n*stencilValue) * currentVertex->coords + stencilValue * sumStarPts;


    return vertexPt;

}

QVector3D edgePoint(HalfEdge* firstEdge) {
    QVector3D EdgePt;
    HalfEdge* currentEdge;

    EdgePt = QVector3D();
    currentEdge = firstEdge;


    EdgePt = QVector3D();
    currentEdge = firstEdge;
    EdgePt  = 6.0 * currentEdge->target->coords;
    EdgePt += 2.0 * currentEdge->next->target->coords;
    EdgePt += 6.0 * currentEdge->twin->target->coords;
    EdgePt += 2.0 * currentEdge->twin->next->target->coords;
    EdgePt /= 16.0;

    return EdgePt;

}

void Mesh::splitHalfEdges(QVector<Vertex>& newVertices, QVector<HalfEdge>& newHalfEdges) {
    unsigned int vIndex = vertices.size();

    for (unsigned int k = 0; k < halfEdges.size(); ++k) {
        HalfEdge* currentEdge = &halfEdges[k];
        unsigned int m = currentEdge->twin->index;

        // Target, Next, Prev, Twin, Poly, Index
        newHalfEdges.append(HalfEdge(nullptr,
                                              nullptr,
                                              nullptr,
                                              nullptr,
                                              nullptr,
                                              2*k));

        newHalfEdges.append(HalfEdge(nullptr,
                                              nullptr,
                                              nullptr,
                                              nullptr,
                                              nullptr,
                                              2*k+1));

        if (k < m) {
            newHalfEdges[2*k].target = &newVertices[ vIndex ];
            newHalfEdges[2*k+1].target = &newVertices[ currentEdge->target->index ];
            vIndex++;
        }
        else {
            newHalfEdges[2*k].target = newHalfEdges[2*m].target;
            newHalfEdges[2*k+1].target = &newVertices[ currentEdge->target->index ];

            // Assign Twinss
            newHalfEdges[2*k].twin = &newHalfEdges[2*m+1];
            newHalfEdges[2*k+1].twin = &newHalfEdges[2*m];
            newHalfEdges[2*m].twin = &newHalfEdges[2*k+1];
            newHalfEdges[2*m+1].twin = &newHalfEdges[2*k];
        }
    }

    // Note that Next, Prev and Poly are not yet assigned at this point.

}
