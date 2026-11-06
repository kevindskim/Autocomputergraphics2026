// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "geometryengine.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QTimer>
#include <QElapsedTimer>

class GeometryEngine;

class MainWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

    QSize sizeHint() const override;

public slots:
    // cube-final: called from the menu bar and the keyboard
    void resetScene();
    void setLightingEnabled(bool enabled);
    void setLightColor(const QVector3D &color);
    void setPerspectiveProjection(bool perspective);
    void showDiceFace(int number);   // turn the face with this number towards the camera
    void setCameraFollowsObject(bool follows);   // camera fixed in the object frame / world frame
    void setFieldOfView(float degrees);          // vertical field of view: wide angle <-> telephoto
    void setShape(int shape);                    // GeometryEngine::Shape as int (settings dialog)
    void setDieView(bool enabled);               // first-person view from the die's face 1

public:
    float currentFieldOfView() const { return fieldOfView; }
    bool isDieView() const { return dieView; }
    int faceCount() const { return geometries ? geometries->faceCount() : 6; }

signals:
    void statusMessage(const QString &message);
    void fieldOfViewChanged(float degrees);
    void dieViewChanged(bool enabled);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void initShaders();
    void initTextures();

private:
    QOpenGLShaderProgram program;
    QOpenGLShaderProgram lineProgram;   // cube-final: colored reference lines (grid, axes)
    GeometryEngine *geometries = nullptr;

    QOpenGLTexture *texture = nullptr;
    QOpenGLTexture *numberTexture = nullptr;   // cube-final: generated numbers for the polyhedra

    // cube-final: first-person view from the die (the real camera is drawn as a model)
    bool dieView = false;

    QMatrix4x4 projection;

    // Computed in paintGL()
    QMatrix4x4 modelmatrix;       // object frame -> world frame
    QMatrix4x4 cameraposmatrix;   // view(camera) frame -> world frame

    QVector2D lastMousePosition;

    // Object pose (object frame w.r.t. world frame), initialized in initScene()
    QVector3D objectPosition;
    QQuaternion objectOrientation;

    // Camera pose (eye, look-at target, up vector in world frame), initialized in initScene()
    QVector3D cameraPosition;
    QVector3D cameraTarget;
    QVector3D cameraUp;

    // cube-final: projection mode
    bool perspectiveProjection = true;

    // cube-final: point light orbiting the world y axis
    bool lightingEnabled = true;
    QVector3D lightColor = QVector3D(1.0f, 1.0f, 1.0f);
    float lightAngle = 45.0f;     // degrees about the world y axis

    // cube-final: smooth rotation used by showDiceFace()
    QTimer animationTimer;
    QQuaternion animationFrom;
    QQuaternion animationTo;
    float animationT = 1.0f;

    // cube-final: vertical field of view of the perspective projection, initialized in initScene()
    float fieldOfView = 45.0f;

    // cube-final: camera frame mode
    // false: camera is fixed in the world frame (the cube turns on screen)
    // true : camera is fixed in the object frame (the cube stays still on screen, the world turns)
    bool cameraFollowsObject = false;

    // showDiceFace() in object-frame mode moves the camera instead of the cube
    bool animateCamera = false;
    QQuaternion animationCamRot;
    QVector3D animationCamPos;
    QVector3D animationCamUp;

    // cube-final: inertia, the cube keeps spinning after the mouse is released
    QTimer inertiaTimer;
    QElapsedTimer dragClock;      // time between mouse move events
    QVector3D inertiaAxis;        // rotation axis in the view frame
    float inertiaSpeed = 0.0f;    // degrees per second

    // cube-final: two digits typed quickly make one face number (1 then 2 -> face 12)
    QElapsedTimer digitClock;
    int typedNumber = 0;

    void initScene();
    void updateProjection();
    QMatrix4x4 viewMatrix() const;         // the view used for drawing (camera or die)
    QMatrix4x4 cameraViewMatrix() const;   // always the real camera
    QVector3D lightPosition() const;

    void rotateObject(const QVector3D &n_view, float angle);
    void rotateCameraAboutObject(const QQuaternion &q);
    void orbitCamera(float degrees);
    void pitchCamera(float degrees);
    void zoomCamera(float delta);
    void animationStep();
    void inertiaStep();
    void stopInertia();
};

#endif // MAINWIDGET_H
