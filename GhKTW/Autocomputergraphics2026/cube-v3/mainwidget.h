// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "geometryengine.h"
#include "transformgizmo.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>

class GeometryEngine;

// Values are passed to the fragment shader as the render_mode uniform
enum class RenderMode
{
    Wireframe = 0,
    Solid = 1,
    Textured = 2
};

class MainWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    using QOpenGLWidget::QOpenGLWidget;
    ~MainWidget();

    QSize sizeHint() const override;

public slots:
    void setRenderMode(RenderMode mode);
    void resetView();
    void viewAlongAxis(const QVector3D &direction);
    void setRotateHandles(bool rotate);

signals:
    // Camera axes in the world frame, sent whenever the camera angle may have changed
    void cameraChanged(const QVector3D &right, const QVector3D &up);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void initShaders();
    void initShaderProgram(QOpenGLShaderProgram &shaderProgram, const QString &vertexShaderFile,
                           const QString &fragmentShaderFile);
    void initTextures();

private:
    QOpenGLShaderProgram program;
    QOpenGLShaderProgram gridProgram;
    GeometryEngine *geometries = nullptr;

    QOpenGLTexture *texture = nullptr;

    QMatrix4x4 projection;

    QVector2D lastMousePosition;

    // A press becomes a click when the mouse is released without moving away
    QPointF pressPosition;
    bool clickPending = false;

    bool objectSelected = false;
    TransformGizmo gizmo;

    // Object pose (object frame w.r.t. world frame), initialized in initScene()
    QVector3D objectPosition;
    QQuaternion objectOrientation;

    // Camera orbit around cameraTarget (degrees and world units), initialized in initScene()
    QVector3D cameraTarget;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float cameraDistance = 0.0f;

    // Camera pose in the world frame, always derived from the values above
    // in updateCameraPosition()
    QVector3D cameraPosition;
    QVector3D cameraRight;
    QVector3D cameraUp;

    RenderMode renderMode = RenderMode::Textured;

    void initScene();
    void updateCameraPosition();
    void panCamera(const QVector2D &diff);

    QMatrix4x4 objectMatrix() const;
    QMatrix4x4 viewMatrix() const;
    void updateGizmoPlacement();
    TransformGizmo::Handle gizmoHandleAt(const QPointF &position);
    bool rayHitsObject(const TransformGizmo::Ray &ray) const;
};

#endif // MAINWIDGET_H
