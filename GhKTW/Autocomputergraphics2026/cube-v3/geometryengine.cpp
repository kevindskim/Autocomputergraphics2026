// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "geometryengine.h"

#include <QVector2D>
#include <QVector3D>

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

    // The grid has its own VAO and VBO because its vertex layout is different
    if (!gridVertexArrayObject.create())
        qFatal("Failed to create grid vertex array object");
    gridArrayBuf.create();

    initGridGeometry();

    // The gizmo changes with the camera, so its VBO is filled again for every frame
    if (!gizmoVertexArrayObject.create())
        qFatal("Failed to create gizmo vertex array object");
    gizmoArrayBuf.create();
    gizmoArrayBuf.setUsagePattern(QOpenGLBuffer::DynamicDraw);
}

GeometryEngine::~GeometryEngine()
{
    vertexArrayObject.destroy();
    arrayBuf.destroy();
    indexBuf.destroy();
    gridVertexArrayObject.destroy();
    gridArrayBuf.destroy();
    gizmoVertexArrayObject.destroy();
    gizmoArrayBuf.destroy();
}
//! [0]

void GeometryEngine::initCubeGeometry()
{
    // For cube we would need only 8 vertices but we have to
    // duplicate vertex for each face because texture coordinate
    // and normal are different.
    const QVector3D nFront( 0.0f,  0.0f,  1.0f);
    const QVector3D nRight( 1.0f,  0.0f,  0.0f);
    const QVector3D nBack(  0.0f,  0.0f, -1.0f);
    const QVector3D nLeft( -1.0f,  0.0f,  0.0f);
    const QVector3D nDown(  0.0f, -1.0f,  0.0f);
    const QVector3D nUp(    0.0f,  1.0f,  0.0f);

    VertexData vertices[] = {
        // Vertex data for face 0
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f), nFront},  // v0
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D(0.33f, 0.0f), nFront}, // v1
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.5f), nFront},  // v2
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.33f, 0.5f), nFront}, // v3

        // Vertex data for face 1
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D( 0.0f, 0.5f), nRight}, // v4
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.33f, 0.5f), nRight}, // v5
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.0f, 1.0f), nRight},  // v6
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.33f, 1.0f), nRight}, // v7

        // Vertex data for face 2
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.5f), nBack},  // v8
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(1.0f, 0.5f), nBack},   // v9
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.66f, 1.0f), nBack},  // v10
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(1.0f, 1.0f), nBack},   // v11

        // Vertex data for face 3
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.0f), nLeft},  // v12
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(1.0f, 0.0f), nLeft},   // v13
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.66f, 0.5f), nLeft},  // v14
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(1.0f, 0.5f), nLeft},   // v15

        // Vertex data for face 4
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.33f, 0.0f), nDown},  // v16
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.0f), nDown},  // v17
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.33f, 0.5f), nDown},  // v18
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D(0.66f, 0.5f), nDown},  // v19

        // Vertex data for face 5
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.33f, 0.5f), nUp},    // v20
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.66f, 0.5f), nUp},    // v21
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.33f, 1.0f), nUp},    // v22
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.66f, 1.0f), nUp}     // v23
    };

    // Indices for drawing cube faces using triangle strips.
    // Triangle strips can be connected by duplicating indices
    // between the strips. If connecting strips have opposite
    // vertex order then last index of the first strip and first
    // index of the second strip needs to be duplicated. If
    // connecting strips have same vertex order then only last
    // index of the first strip needs to be duplicated.
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
}

// Lines of the floor on the world xz plane, just below the cube so that they
// do not fight with its bottom face in the depth buffer
void GeometryEngine::initGridGeometry()
{
    const int halfSize = 20;
    const float floorY = -1.01f;

    const QVector3D lineColor(0.34f, 0.34f, 0.34f);
    const QVector3D xAxisColor(0.90f, 0.31f, 0.35f);    // Line along x, where z = 0
    const QVector3D zAxisColor(0.25f, 0.55f, 0.90f);    // Line along z, where x = 0

    std::vector<ColorVertex> vertices;

    for (int i = -halfSize; i <= halfSize; ++i) {
        const float offset = i;
        const float end = halfSize;

        // Line along x at z = offset
        const QVector3D alongX = i == 0 ? xAxisColor : lineColor;
        vertices.push_back({QVector3D(-end, floorY, offset), alongX});
        vertices.push_back({QVector3D( end, floorY, offset), alongX});

        // Line along z at x = offset
        const QVector3D alongZ = i == 0 ? zAxisColor : lineColor;
        vertices.push_back({QVector3D(offset, floorY, -end), alongZ});
        vertices.push_back({QVector3D(offset, floorY,  end), alongZ});
    }

    gridVertexCount = static_cast<int>(vertices.size());

    gridArrayBuf.bind();
    gridArrayBuf.allocate(vertices.data(), gridVertexCount * sizeof(ColorVertex));
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

    // Draw cube geometry using indices from VBO 1
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);
}
//! [2]

void GeometryEngine::drawGridGeometry(QOpenGLShaderProgram *program)
{
    QOpenGLVertexArrayObject::Binder vertexArrayBinder(&gridVertexArrayObject);

    gridArrayBuf.bind();
    setColorVertexAttributes(program);

    glDrawArrays(GL_LINES, 0, gridVertexCount);
}

void GeometryEngine::drawGizmoGeometry(QOpenGLShaderProgram *program, const std::vector<ColorVertex> &vertices)
{
    QOpenGLVertexArrayObject::Binder vertexArrayBinder(&gizmoVertexArrayObject);

    const int vertexCount = static_cast<int>(vertices.size());

    gizmoArrayBuf.bind();
    gizmoArrayBuf.allocate(vertices.data(), vertexCount * sizeof(ColorVertex));
    setColorVertexAttributes(program);

    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

void GeometryEngine::setColorVertexAttributes(QOpenGLShaderProgram *program)
{
    int vertexLocation = program->attributeLocation("a_position");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(ColorVertex));

    int colorLocation = program->attributeLocation("a_color");
    program->enableAttributeArray(colorLocation);
    program->setAttributeBuffer(colorLocation, GL_FLOAT, sizeof(QVector3D), 3, sizeof(ColorVertex));
}
