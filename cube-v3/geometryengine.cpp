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
}

GeometryEngine::~GeometryEngine()
{
    vertexArrayObject.destroy();
    arrayBuf.destroy();
    indexBuf.destroy();
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
    offset += sizeof(QVector2D); // Advance past texture coordinates to reach normal data

    int normalLocation = program->attributeLocation("a_normal");
    program->enableAttributeArray(normalLocation);
    program->setAttributeBuffer(normalLocation, GL_FLOAT, offset, 3, sizeof(VertexData));


    // Draw cube geometry using indices from VBO 1
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);
}
//! [2]
