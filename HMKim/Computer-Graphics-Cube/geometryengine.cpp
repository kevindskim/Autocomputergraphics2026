// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "geometryengine.h"

#include <QVector>
#include <QVector2D>
#include <QVector3D>

#include <cmath>
#include <utility>

// cube-final: a face normal is added to each vertex for lighting
struct VertexData
{
    QVector3D position;
    QVector2D texCoord;
    QVector3D normal;
};

//! [0]
GeometryEngine::GeometryEngine()
    : indexBuf(QOpenGLBuffer::IndexBuffer)
{
    if (!initializeOpenGLFunctions())
        qFatal("Failed to initialize OpenGL 3.3 Core geometry functions");

    // Generate the VAO and 2 VBOs required by the core profile.
    if (!vertexArrayObject.create())
        qFatal("Failed to create vertex array object");
    arrayBuf.create();
    indexBuf.create();

    // Initializes cube geometry and transfers it to VBOs
    initCubeGeometry();

    // Reference lines for the world / object frames
    if (!lineArrayObject.create())
        qFatal("Failed to create line vertex array object");
    lineBuf.create();
    initLineGeometry();
}

GeometryEngine::~GeometryEngine()
{
    vertexArrayObject.destroy();
    arrayBuf.destroy();
    indexBuf.destroy();
    lineArrayObject.destroy();
    lineBuf.destroy();
}
//! [0]

void GeometryEngine::initCubeGeometry()
{
    // For cube we would need only 8 vertices but we have to
    // duplicate vertex for each face because texture coordinate
    // and normal are different.
    const QVector3D nPX( 1.0f,  0.0f,  0.0f), nNX(-1.0f,  0.0f,  0.0f);
    const QVector3D nPY( 0.0f,  1.0f,  0.0f), nNY( 0.0f, -1.0f,  0.0f);
    const QVector3D nPZ( 0.0f,  0.0f,  1.0f), nNZ( 0.0f,  0.0f, -1.0f);

    VertexData vertices[] = {
        // Vertex data for face 0 (+z, dice 4)
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f),  nPZ}, // v0
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D(0.33f, 0.0f), nPZ}, // v1
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.5f),  nPZ}, // v2
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.33f, 0.5f), nPZ}, // v3

        // Vertex data for face 1 (+x, dice 1)
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D( 0.0f, 0.5f), nPX}, // v4
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.33f, 0.5f), nPX}, // v5
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.0f, 1.0f),  nPX}, // v6
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.33f, 1.0f), nPX}, // v7

        // Vertex data for face 2 (-z, dice 3)
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.5f), nNZ}, // v8
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(1.0f, 0.5f),  nNZ}, // v9
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.66f, 1.0f), nNZ}, // v10
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(1.0f, 1.0f),  nNZ}, // v11

        // Vertex data for face 3 (-x, dice 6)
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.0f), nNX}, // v12
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(1.0f, 0.0f),  nNX}, // v13
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.66f, 0.5f), nNX}, // v14
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(1.0f, 0.5f),  nNX}, // v15

        // Vertex data for face 4 (-y, dice 5)
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.33f, 0.0f), nNY}, // v16
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.0f), nNY}, // v17
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.33f, 0.5f), nNY}, // v18
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D(0.66f, 0.5f), nNY}, // v19

        // Vertex data for face 5 (+y, dice 2)
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.33f, 0.5f), nPY}, // v20
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.66f, 0.5f), nPY}, // v21
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.33f, 1.0f), nPY}, // v22
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.66f, 1.0f), nPY}  // v23
    };

    // Two triangles (GL_TRIANGLES) per face, counter-clockwise seen from outside
   GLushort indices[] = {
     0,  1,  2,   2,  1,  3,  // Face 0
     4,  5,  6,   6,  5,  7,  // Face 1
     8,  9, 10,  10,  9, 11,  // Face 2
    12, 13, 14,  14, 13, 15,  // Face 3
    16, 17, 18,  18, 17, 19,  // Face 4
    20, 21, 22,  22, 21, 23   // Face 5
};

//! [1]
    // Transfer vertex data to VBO 0
    arrayBuf.bind();
    arrayBuf.allocate(vertices, 24 * sizeof(VertexData));

    // Transfer index data to VBO 1
    indexBuf.bind();
    indexBuf.allocate(indices, 36 * sizeof(GLushort));
//! [1]

    // Face normals by dice number (see the face comments above)
    faceNormals = {
        QVector3D( 1,  0,  0),   // 1 : Face 1
        QVector3D( 0,  1,  0),   // 2 : Face 5
        QVector3D( 0,  0, -1),   // 3 : Face 2
        QVector3D( 0,  0,  1),   // 4 : Face 0
        QVector3D( 0, -1,  0),   // 5 : Face 4
        QVector3D(-1,  0,  0),   // 6 : Face 3
    };
    indexCount = 36;
    boundingRadius = std::sqrt(3.0f);
}

void GeometryEngine::setShape(Shape shape)
{
    currentShape = shape;

    // Golden ratio for the icosahedron
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

    switch (shape) {
    case Shape::Cube:
        initCubeGeometry();
        break;
    case Shape::Tetrahedron:
        // Every other corner of a cube, edge length 2*sqrt(2)
        initPolyhedron({QVector3D( 1,  1,  1), QVector3D( 1, -1, -1),
                        QVector3D(-1,  1, -1), QVector3D(-1, -1,  1)},
                       2.0f * std::sqrt(2.0f), 1.9f);
        break;
    case Shape::Octahedron:
        // Unit points on the three axes, edge length sqrt(2)
        initPolyhedron({QVector3D( 1, 0, 0), QVector3D(-1, 0, 0),
                        QVector3D( 0, 1, 0), QVector3D( 0, -1, 0),
                        QVector3D( 0, 0, 1), QVector3D( 0, 0, -1)},
                       std::sqrt(2.0f), 1.7f);
        break;
    case Shape::Icosahedron: {
        // Three orthogonal golden rectangles, edge length 2
        QVector<QVector3D> c;
        for (float a : {-1.0f, 1.0f}) {
            for (float b : {-phi, phi}) {
                c.append(QVector3D(0, a, b));
                c.append(QVector3D(a, b, 0));
                c.append(QVector3D(b, 0, a));
            }
        }
        initPolyhedron(c, 2.0f, 1.7f);
        break;
    }
    }
}

QVector3D GeometryEngine::faceNormal(int number) const
{
    if (number < 1 || number > faceNormals.size())
        return QVector3D();
    return faceNormals.at(number - 1);
}

// Builds a regular polyhedron with triangular faces from its corner points.
// Faces are found as the triples of corners whose three sides all equal the edge length.
void GeometryEngine::initPolyhedron(const QVector<QVector3D> &corners, float edge, float radius)
{
    struct Face { QVector3D a, b, c, normal; int number = 0; };
    QVector<Face> faces;

    auto isEdge = [edge](const QVector3D &p, const QVector3D &q) {
        return std::abs((p - q).length() - edge) < 1e-3f * edge;
    };

    const int n = int(corners.size());
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            for (int k = j + 1; k < n; ++k) {
                QVector3D a = corners.at(i), b = corners.at(j), c = corners.at(k);
                if (!isEdge(a, b) || !isEdge(b, c) || !isEdge(c, a))
                    continue;

                // Counter-clockwise seen from outside: the normal must point away from the center
                QVector3D normal = QVector3D::crossProduct(b - a, c - a);
                if (QVector3D::dotProduct(normal, a + b + c) < 0.0f) {
                    std::swap(b, c);
                    normal = -normal;
                }
                faces.append({a, b, c, normal.normalized()});
            }

    // Number the faces like a real die: opposite faces add up to (face count + 1)
    const int count = int(faces.size());
    int next = 1;
    for (Face &f : faces) {
        if (f.number != 0)
            continue;
        f.number = next;
        for (Face &g : faces)
            if (g.number == 0 && QVector3D::dotProduct(f.normal, g.normal) < -0.999f)
                g.number = count + 1 - next;
        ++next;
    }

    // Three vertices per face (flat normal), texture cell = face number - 1
    QVector<VertexData> vertices;
    QVector<GLushort> indices;
    faceNormals.resize(count);
    const float cw = 1.0f / AtlasColumns, ch = 1.0f / AtlasRows;
    for (const Face &f : faces) {
        const int cell = f.number - 1;
        const float u0 = (cell % AtlasColumns) * cw;
        const float v0 = 1.0f - (cell / AtlasColumns + 1) * ch;   // row 0 is the top row of the image

        // Same triangle as the one drawn in the number texture (MainWidget::initTextures)
        const QVector2D ta(u0 + 0.1f * cw, v0 + 0.12f * ch);
        const QVector2D tb(u0 + 0.9f * cw, v0 + 0.12f * ch);
        const QVector2D tc(u0 + 0.5f * cw, v0 + 0.92f * ch);

        const float s = radius / f.a.length();   // scale corners onto the bounding sphere
        indices << GLushort(vertices.size()) << GLushort(vertices.size() + 1) << GLushort(vertices.size() + 2);
        vertices << VertexData{f.a * s, ta, f.normal}
                 << VertexData{f.b * s, tb, f.normal}
                 << VertexData{f.c * s, tc, f.normal};

        faceNormals[cell] = f.normal;
    }

    arrayBuf.bind();
    arrayBuf.allocate(vertices.constData(), int(vertices.size() * sizeof(VertexData)));
    indexBuf.bind();
    indexBuf.allocate(indices.constData(), int(indices.size() * sizeof(GLushort)));

    indexCount = int(indices.size());
    boundingRadius = radius;
}

struct LineVertex
{
    QVector3D position;
    QVector3D color;
};

void GeometryEngine::initLineGeometry()
{
    QVector<LineVertex> v;
    auto line = [&v](const QVector3D &a, const QVector3D &b, const QVector3D &color) {
        v.append({a, color});
        v.append({b, color});
    };

    // Floor grid on the world plane y = -2, from -6 to 6
    const float floorY = -2.0f, half = 6.0f;
    const QVector3D gridColor(0.35f, 0.37f, 0.42f);
    for (int i = -6; i <= 6; ++i) {
        const float t = float(i);
        line(QVector3D(t, floorY, -half), QVector3D(t, floorY, half), gridColor);
        line(QVector3D(-half, floorY, t), QVector3D(half, floorY, t), gridColor);
    }

    // World axes on the floor origin: x red, y green, z blue
    const QVector3D o(0.0f, floorY + 0.01f, 0.0f);
    line(o, o + QVector3D(3, 0, 0), QVector3D(1.0f, 0.2f, 0.2f));
    line(o, o + QVector3D(0, 3, 0), QVector3D(0.2f, 1.0f, 0.2f));
    line(o, o + QVector3D(0, 0, 3), QVector3D(0.3f, 0.5f, 1.0f));
    worldLineCount = v.size();

    // Object axes from the cube center, sticking out of the faces: lighter x / y / z colors
    line(QVector3D(), QVector3D(2, 0, 0), QVector3D(1.0f, 0.6f, 0.6f));
    line(QVector3D(), QVector3D(0, 2, 0), QVector3D(0.6f, 1.0f, 0.6f));
    line(QVector3D(), QVector3D(0, 0, 2), QVector3D(0.6f, 0.75f, 1.0f));
    objectAxesCount = v.size() - worldLineCount;

    // Camera model in the camera frame (looks along -z, up is +y): body box, lens frustum, up mark
    const QVector3D camColor(1.0f, 0.85f, 0.2f);
    const float bx = 0.35f, by = 0.25f, bz = 0.5f;
    for (float x : {-bx, bx})
        for (float y : {-by, by})
            line(QVector3D(x, y, 0), QVector3D(x, y, bz), camColor);      // box edges along z
    for (float z : {0.0f, bz}) {
        line(QVector3D(-bx, -by, z), QVector3D( bx, -by, z), camColor);
        line(QVector3D(-bx,  by, z), QVector3D( bx,  by, z), camColor);
        line(QVector3D(-bx, -by, z), QVector3D(-bx,  by, z), camColor);
        line(QVector3D( bx, -by, z), QVector3D( bx,  by, z), camColor);
    }
    const float fx = 0.6f, fy = 0.45f, fz = -0.7f;
    for (float sx : {-1.0f, 1.0f})
        for (float sy : {-1.0f, 1.0f})
            line(QVector3D(sx * bx, sy * by, 0), QVector3D(sx * fx, sy * fy, fz), camColor);
    line(QVector3D(-fx, -fy, fz), QVector3D( fx, -fy, fz), camColor);
    line(QVector3D(-fx,  fy, fz), QVector3D( fx,  fy, fz), camColor);
    line(QVector3D(-fx, -fy, fz), QVector3D(-fx,  fy, fz), camColor);
    line(QVector3D( fx, -fy, fz), QVector3D( fx,  fy, fz), camColor);
    line(QVector3D(-0.15f, by, 0.25f), QVector3D(0.0f, by + 0.2f, 0.25f), camColor);   // up mark
    line(QVector3D( 0.15f, by, 0.25f), QVector3D(0.0f, by + 0.2f, 0.25f), camColor);
    cameraModelCount = v.size() - worldLineCount - objectAxesCount;

    lineBuf.bind();
    lineBuf.allocate(v.constData(), int(v.size() * sizeof(LineVertex)));
}

void GeometryEngine::drawLines(QOpenGLShaderProgram *program, int first, int count)
{
    QOpenGLVertexArrayObject::Binder vertexArrayBinder(&lineArrayObject);
    lineBuf.bind();

    int vertexLocation = program->attributeLocation("a_position");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(LineVertex));

    int colorLocation = program->attributeLocation("a_color");
    program->enableAttributeArray(colorLocation);
    program->setAttributeBuffer(colorLocation, GL_FLOAT, sizeof(QVector3D), 3, sizeof(LineVertex));

    glDrawArrays(GL_LINES, first, count);
}

void GeometryEngine::drawWorldLines(QOpenGLShaderProgram *program)
{
    drawLines(program, 0, worldLineCount);
}

void GeometryEngine::drawObjectAxes(QOpenGLShaderProgram *program)
{
    drawLines(program, worldLineCount, objectAxesCount);
}

void GeometryEngine::drawCameraModel(QOpenGLShaderProgram *program)
{
    drawLines(program, worldLineCount + objectAxesCount, cameraModelCount);
}

//! [2]
void GeometryEngine::drawCubeGeometry(QOpenGLShaderProgram *program)
{
    QOpenGLVertexArrayObject::Binder vertexArrayBinder(&vertexArrayObject);

    // Tell OpenGL which VBOs to use
    arrayBuf.bind();
    indexBuf.bind();

    // Offset for position
    quintptr offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("a_position");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    int texcoordLocation = program->attributeLocation("a_texcoord");
    program->enableAttributeArray(texcoordLocation);
    program->setAttributeBuffer(texcoordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    // Offset for normal
    offset += sizeof(QVector2D);

    // Tell OpenGL programmable pipeline how to locate vertex normal data
    int normalLocation = program->attributeLocation("a_normal");
    program->enableAttributeArray(normalLocation);
    program->setAttributeBuffer(normalLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Draw the current shape using indices from VBO 1
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, nullptr);
}
//! [2]
