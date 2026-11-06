// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector>
#include <QVector3D>

class GeometryEngine : protected QOpenGLFunctions_3_3_Core
{
public:
    // cube-final: shapes that can be chosen in the settings dialog
    enum class Shape { Cube, Tetrahedron, Octahedron, Icosahedron };

    // Layout of the generated number texture (one cell per face number, up to 20)
    static constexpr int AtlasColumns = 5;
    static constexpr int AtlasRows = 4;

    GeometryEngine();
    virtual ~GeometryEngine();

    // Draws the current shape (cube or polyhedron)
    void drawCubeGeometry(QOpenGLShaderProgram *program);

    // cube-final: rebuild the VBOs for another shape (needs a current GL context)
    void setShape(Shape shape);
    Shape shape() const { return currentShape; }
    int faceCount() const { return int(faceNormals.size()); }
    QVector3D faceNormal(int number) const;   // outward normal of the face with this number, object frame
    float radius() const { return boundingRadius; }

    // cube-final: reference lines (position + color, GL_LINES)
    void drawWorldLines(QOpenGLShaderProgram *program);   // floor grid + world axes, world frame
    void drawObjectAxes(QOpenGLShaderProgram *program);   // object axes, object frame
    void drawCameraModel(QOpenGLShaderProgram *program);  // camera body + view frustum, camera frame

private:
    void initCubeGeometry();
    void initPolyhedron(const QVector<QVector3D> &corners, float edge, float radius);
    void initLineGeometry();
    void drawLines(QOpenGLShaderProgram *program, int first, int count);

    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;
    QOpenGLVertexArrayObject vertexArrayObject;

    Shape currentShape = Shape::Cube;
    QVector<QVector3D> faceNormals;   // index = face number - 1
    int indexCount = 0;
    float boundingRadius = 0.0f;      // distance from the center to the farthest vertex

    QOpenGLBuffer lineBuf;
    QOpenGLVertexArrayObject lineArrayObject;
    int worldLineCount = 0;       // vertices of the grid + world axes
    int objectAxesCount = 0;      // vertices of the object axes, stored after the world lines
    int cameraModelCount = 0;     // vertices of the camera model, stored after the object axes
};

#endif // GEOMETRYENGINE_H
