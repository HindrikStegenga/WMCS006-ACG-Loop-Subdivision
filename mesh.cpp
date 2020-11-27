#include "mesh.h"
#include "math.h"

Mesh::Mesh() {
    qDebug() << "✓✓ Mesh constructor (Empty)";
}

Mesh::Mesh(OBJFile* loadedOBJFile) {

    qDebug() << "✓✓ Mesh constructor (OBJ)";

    // Convert loaded OBJ file to HalfEdge mesh
    unsigned int numVertices, numHalfEdges, numFaces;

    numVertices = loadedOBJFile->vertexCoords.size();
    numHalfEdges = 0;

    for (int k = 0; k < loadedOBJFile->faceValences.size(); k++) {
        numHalfEdges += loadedOBJFile->faceValences[k];
    }

    numFaces = loadedOBJFile->faceValences.size();

    // Note - resize() invokes the Vertex() constructor, reserve() does not.
    vertices.reserve(numVertices);
    // If boundaries are present, reserve twice as much = worst case scenario
    halfEdges.reserve(2*numHalfEdges);
    faces.reserve(numFaces);

    // Add Vertices

    for (int k = 0; k < numVertices; k++) {
        // Coords (x,y,z), Out, Valence, Index
        vertices.append(Vertex(loadedOBJFile->vertexCoords[k],
                               nullptr,
                               0,
                               k));
        // Out and valence are unknown at this point.
    }

    qDebug() << "   # Vertices" << vertices.capacity() << vertices.size();

    unsigned int indexH = 0;
    unsigned int currentIndex = 0;

    // Initialize every entry of PotentialTwins with an empty QVector (using resize() )
    QVector<QVector<unsigned int>> potentialTwins = QVector<QVector<unsigned int>>(loadedOBJFile->vertexCoords.size());
    //potentialTwins.resize(loadedOBJFile->vertexCoords.size());

    // Add Faces and most of the HalfEdges
    unsigned int n;
    for (unsigned int m = 0; m < numFaces; m++) {
        // Side, Val, Index
        faces.append(Face(nullptr,
                          loadedOBJFile->faceValences[m],
                          m));

        for (n = 0; n < loadedOBJFile->faceValences[m]; n++) {
            // Target, Next, Prev, Twin, Poly, Index
            halfEdges.append(HalfEdge(&vertices[loadedOBJFile->faceCoordInd[currentIndex+n]],
                    nullptr,
                    nullptr,
                    nullptr,
                    &faces[m],
                    indexH));

            // Next, Prev and Twin of the above HalfEdge have to be assigned later! Starting below...

            if (n > 0) {
                halfEdges[indexH-1].next = &halfEdges[indexH];
                halfEdges[indexH].prev = &halfEdges[indexH-1];

                // Append index of HalfEdge to list of OutgoingHalfEdges of its TailVertex.
                potentialTwins[loadedOBJFile->faceCoordInd[currentIndex+n-1]].append(indexH);
            }
            indexH++;
        }

        // HalfEdges[indexH-1] is the most recent addition.
        faces[m].side = &halfEdges[indexH-1];

        halfEdges[indexH-1].next = &halfEdges[indexH-n];
        halfEdges[indexH-n].prev = &halfEdges[indexH-1];

        potentialTwins[loadedOBJFile->faceCoordInd[currentIndex+n-1]].append(indexH-n);

        currentIndex += loadedOBJFile->faceValences[m];
    }

    qDebug() << "   # Faces" << faces.capacity() << faces.size();
    qDebug() << "   # HalfEdges" << halfEdges.capacity() << halfEdges.size();

    // Outs and Valences of vertices
    for (int k = 0; k < vertices.size(); k++) {
        if (potentialTwins[k].size() == 0) {
            qWarning() << " ! Isolated Vertex? PotentialTwins empty for Index" << k;
            dispVertInfo(vertices[k]);
            continue;
        }
        vertices[k].out = &halfEdges[potentialTwins[k][0]];
        // Not the correct valence when on the boundary! Fixed below.
        vertices[k].val = potentialTwins[k].size();
    }

    setTwins(numHalfEdges, indexH, potentialTwins);


    qDebug() << "   # Updated HalfEdges" << halfEdges.capacity() << halfEdges.size();
}

Mesh::~Mesh() {
    qDebug() << "✗✗ Mesh destructor";

    qDebug() << "   # Vertices:" << vertices.size();
    qDebug() << "   # HalfEdges:" << halfEdges.size();
    qDebug() << "   # Faces:" << faces.size();

    vertices.clear();
    vertices.squeeze();
    halfEdges.clear();
    halfEdges.squeeze();
    faces.clear();
    faces.squeeze();
}

void Mesh::extractAttributes() {
    unsigned int k;
    unsigned short m;
    HalfEdge* currentEdge;

    vertexCoords.clear();
    vertexCoords.reserve(vertices.size());

    for (k = 0; k < vertices.size(); k++) {
        vertexCoords.append(vertices[k].coords);
    }

    vertexNormals.clear();
    vertexNormals.reserve(vertices.size());

    for (k = 0; k < faces.size(); k++) {
        setFaceNormal(faces[k]);
    }

    for (k = 0; k < vertices.size(); k++) {
        vertexNormals.append(computeVertexNormal(vertices[k]) );
    }

    polyIndices.clear();
    polyIndices.reserve(halfEdges.size() + faces.size());

    for (k = 0; k < faces.size(); k++) {
        currentEdge = faces[k].side;
        for (m = 0; m < 3; m++) {
            polyIndices.append(currentEdge->target->index);
            currentEdge = currentEdge->next;
        }
    }
}

void Mesh::setTwins(unsigned int numHalfEdges, unsigned int indexH, QVector<QVector<unsigned int>>& potentialTwins) {
    unsigned int hTail, hHead, len, n;
    QSet<unsigned int> twinless;

    HalfEdge* he;
    // Assign Twins
    for (unsigned int m = 0; m < numHalfEdges; m++) {
        he = &halfEdges[m];
        if (not he->twin) {
            hTail = he->prev->target->index;
            hHead = he->target->index;
            len = he->target->val;
            for (n = 0; n < len; n++) {
                if (halfEdges[potentialTwins[hHead][n]].target->index == hTail) {
                    //qDebug() << "Found Twin!";
                    halfEdges[m].twin = &halfEdges[potentialTwins[hHead][n]];
                    halfEdges[potentialTwins[hHead][n]].twin = he;
                    break;
                }
            }
            if (n == len) {
                // Twin not found...
                twinless.insert(m);
            }
        }
    }

    if (twinless.size() > 0) {
        // The mesh is not closed
        qDebug() << " * There are" << twinless.size() << "HalfEdges without Twin (i.e. the model contains boundaries)";
        //qDebug() << Twinless.values();

        HalfEdge* initialEdge;
        HalfEdge* currentEdge;
        unsigned int startBoundaryLoop;

        while (twinless.size() > 0) {
            // Select a HalfEdge without Twin. The Twin that we will create is part of a boundary edge loop
            qDebug() << " → Processing new Boundary Edge Loop";

            initialEdge = &halfEdges[*twinless.begin()];
            twinless.remove(initialEdge->index);

            // Target, Next, Prev, Twin, Poly, Index
            halfEdges.append(HalfEdge( initialEdge->prev->target,
                                       nullptr,
                                       nullptr,
                                       initialEdge,
                                       nullptr,
                                       indexH ));
            startBoundaryLoop = indexH;
            // Twin of initialEdge should be assigned AFTER the central while loop!
            indexH++;

            // Use a sketch to properly understand these steps (assume counter-clockwise HalfEdges) :)
            currentEdge = initialEdge->prev;
            while (currentEdge->twin != nullptr) {
                currentEdge = currentEdge->twin->prev;
            }

            // Trace the current boundary loop
            while (currentEdge != initialEdge) {
                twinless.remove(currentEdge->index);
                // Target, Next, Prev, Twin, Poly, Index
                halfEdges.append(HalfEdge( currentEdge->prev->target,
                                           nullptr,
                                           &halfEdges[indexH-1],
                                 currentEdge,
                                 nullptr,
                                 indexH ));
                halfEdges[indexH-1].next = &halfEdges[indexH];

                currentEdge->target->val += 1;
                currentEdge->twin = &halfEdges[indexH];
                indexH++;

                currentEdge = currentEdge->prev;
                while (currentEdge->twin != nullptr) {
                    currentEdge = currentEdge->twin->prev;
                }
            }

            halfEdges[startBoundaryLoop].prev = &halfEdges[indexH-1];
            halfEdges[indexH-1].next = &halfEdges[startBoundaryLoop];

            initialEdge->target->val += 1;
            // Set Twin of initialEdge!
            initialEdge->twin = &halfEdges[startBoundaryLoop];
        }

    }
}

void Mesh::setFaceNormal(Face& currentFace) {
    QVector3D faceNormal = QVector3D(0.0, 0.0, 0.0);
    HalfEdge* currentEdge = currentFace.side;

    for (unsigned int k = 0; k < currentFace.val; k++) {
        faceNormal += QVector3D::crossProduct(
                    currentEdge->next->target->coords - currentEdge->target->coords,
                    currentEdge->twin->target->coords - currentEdge->target->coords );
        currentEdge = currentEdge->next;
    }

    currentFace.normal = faceNormal / faceNormal.length();
}

QVector3D Mesh::computeVertexNormal(Vertex& currentVertex) {

    QVector3D vertexNormal = QVector3D();
    HalfEdge* currentEdge = currentVertex.out;
    float faceAngle;
    QVector3D p = currentVertex.coords;

    // this will only work if all outgoing edges of currentvertex
    // have a prev, next and a twin
    for (int k = 0; k < currentVertex.val; k++) {

        faceAngle = acos( fmax(-1.0, QVector3D::dotProduct(
                                   (currentEdge->target->coords - p).normalized(),
                                   (currentEdge->prev->twin->target->coords - p).normalized() ) ) );

        if (currentEdge->polygon) {
            vertexNormal += faceAngle * currentEdge->polygon->normal;
        }

        currentEdge = currentEdge->twin->next;

    }

    return vertexNormal;
}

void Mesh::dispVertInfo(Vertex& dVert) {
    qDebug() << "Vertex at Index =" << dVert.index << "Coords =" << dVert.coords << "Out =" << dVert.out << "Val =" << dVert.val;
}

void Mesh::dispHalfEdgeInfo(HalfEdge& dHalfEdge) {
    qDebug() << "HalfEdge at Index =" << dHalfEdge.index << "Target =" << dHalfEdge.target << "Next =" << dHalfEdge.next << "Prev =" << dHalfEdge.prev << "Twin =" << dHalfEdge.twin << "Poly =" << dHalfEdge.polygon;
}

void Mesh::dispFaceInfo(Face& dFace){
    qDebug() << "Face at Index =" << dFace.index << "Side =" << dFace.side << "Val =" << dFace.val;
}
