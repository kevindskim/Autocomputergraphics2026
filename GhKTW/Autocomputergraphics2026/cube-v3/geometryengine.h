// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>

#include <vector>

// Vertex of the grid and the gizmo
struct ColorVertex
{
    QVector3D position;
    QVector3D color;
};

class GeometryEngine : protected QOpenGLFunctions_3_3_Core
{
public:
    GeometryEngine();
    virtual ~GeometryEngine();

    void drawCubeGeometry(QOpenGLShaderProgram *program);
    void drawGridGeometry(QOpenGLShaderProgram *program);
    void drawGizmoGeometry(QOpenGLShaderProgram *program, const std::vector<ColorVertex> &vertices);

private:
    void initCubeGeometry();
    void initGridGeometry();
    void setColorVertexAttributes(QOpenGLShaderProgram *program);

    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;
    QOpenGLVertexArrayObject vertexArrayObject;

    QOpenGLBuffer gridArrayBuf;
    QOpenGLVertexArrayObject gridVertexArrayObject;
    int gridVertexCount = 0;

    QOpenGLBuffer gizmoArrayBuf;
    QOpenGLVertexArrayObject gizmoVertexArrayObject;
};

#endif // GEOMETRYENGINE_H
