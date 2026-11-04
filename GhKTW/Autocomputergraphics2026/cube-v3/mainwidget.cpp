// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwidget.h"

#include <QDebug>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QWheelEvent>
#include <QtMath>

#include <cmath>
#include <limits>
#include <utility>

// Window size; initial values are used as the size of the window at startup
static int g_windowWidth = 512;
static int g_windowHeight = 512;

// Vertical field of view in degrees
static const float g_fovY = 45.0f;

// Limits of the camera orbit
static const float g_maxPitch = 90.0f;
static const float g_minDistance = 2.0f;
static const float g_maxDistance = 50.0f;

// The cube spans -1 to 1 on every axis of the object frame
static const float g_cubeHalfSize = 1.0f;

// A press moving further than this many pixels is a drag, not a click
static const int g_clickPixels = 3;

QSize MainWidget::sizeHint() const
{
    return QSize(g_windowWidth, g_windowHeight);
}

MainWidget::~MainWidget()
{
    // Make sure the context is current when deleting the texture
    // and the buffers.
    makeCurrent();
    delete texture;
    delete geometries;
    doneCurrent();
}

void MainWidget::setRenderMode(RenderMode mode)
{
    renderMode = mode;
    update();
}

// Back to the initial object and camera poses
void MainWidget::resetView()
{
    gizmo.endDrag();
    initScene();
    update();
}

// Place the camera on the given world direction from the target, looking back at it.
// The target and the distance stay, so the orbit center does not jump.
void MainWidget::viewAlongAxis(const QVector3D &direction)
{
    // Inverse of the spherical coordinates in updateCameraPosition()
    cameraYaw = qRadiansToDegrees(std::atan2(direction.x(), direction.z()));
    cameraPitch = qRadiansToDegrees(std::asin(direction.y()));
    updateCameraPosition();
    update();
}

void MainWidget::setRotateHandles(bool rotate)
{
    gizmo.setMode(rotate ? TransformGizmo::Mode::Rotate : TransformGizmo::Mode::Move);
    update();
}

// Camera state for the gizmo math, taken fresh so that it never depends on the last frame
void MainWidget::updateGizmoPlacement()
{
    TransformGizmo::View view;
    view.viewProjection = projection * viewMatrix();
    view.eye = cameraPosition;
    view.right = cameraRight;
    view.up = cameraUp;
    view.forward = (cameraTarget - cameraPosition).normalized();
    view.tanHalfFov = std::tan(qDegreesToRadians(g_fovY) / 2.0f);
    view.viewport = size();

    gizmo.setPlacement(view, objectPosition);
}

TransformGizmo::Handle MainWidget::gizmoHandleAt(const QPointF &position)
{
    if (!objectSelected)
        return TransformGizmo::Handle::None;

    updateGizmoPlacement();
    return gizmo.handleAt(position);
}

// Slab test: in the object frame the cube is an axis aligned box. The ray is inside the box
// between the last entry into and the first exit out of the three pairs of parallel planes.
bool MainWidget::rayHitsObject(const TransformGizmo::Ray &ray) const
{
    const QMatrix4x4 worldToObject = objectMatrix().inverted();
    const QVector3D origin = worldToObject.map(ray.origin);
    const QVector3D direction = worldToObject.mapVector(ray.direction);

    float tEnter = -std::numeric_limits<float>::max();
    float tExit = std::numeric_limits<float>::max();

    for (int axis = 0; axis < 3; ++axis) {
        // Parallel to this pair of planes: the ray is always between them or never
        if (qAbs(direction[axis]) < 1e-6f) {
            if (qAbs(origin[axis]) > g_cubeHalfSize)
                return false;
            continue;
        }

        float t1 = (-g_cubeHalfSize - origin[axis]) / direction[axis];
        float t2 = ( g_cubeHalfSize - origin[axis]) / direction[axis];
        if (t1 > t2)
            std::swap(t1, t2);

        tEnter = qMax(tEnter, t1);
        tExit = qMin(tExit, t2);
    }

    return tEnter <= tExit && tExit > 0.0f;
}

//! [0]
void MainWidget::mousePressEvent(QMouseEvent *e)
{
    // Save mouse press position
    lastMousePosition = QVector2D(e->position());

    if (e->button() != Qt::LeftButton || (e->modifiers() & Qt::ShiftModifier))
        return;

    pressPosition = e->position();
    clickPending = true;

    // A press on a handle starts moving or rotating the object
    const TransformGizmo::Handle handle = gizmoHandleAt(e->position());
    if (handle != TransformGizmo::Handle::None) {
        gizmo.beginDrag(handle, e->position(), objectPosition, objectOrientation);
        clickPending = false;
        update();
    }
}

void MainWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;

    if (gizmo.isDragging()) {
        gizmo.endDrag();
        update();
    } else if (clickPending) {
        // A click selects what is under the mouse, and clicking elsewhere clears the selection
        updateGizmoPlacement();
        objectSelected = rayHitsObject(gizmo.mouseRay(e->position()));
        update();
    }

    clickPending = false;
}

void MainWidget::mouseMoveEvent(QMouseEvent *e)
{
    // Without a button the mouse only highlights the handle under it
    if (!(e->buttons() & Qt::LeftButton)) {
        gizmo.setHovered(gizmoHandleAt(e->position()));
        update();
        return;
    }

    if ((e->position() - pressPosition).manhattanLength() > g_clickPixels)
        clickPending = false;

    if (gizmo.isDragging()) {
        updateGizmoPlacement();
        gizmo.drag(e->position(), &objectPosition, &objectOrientation);
        update();
        return;
    }

    // Mouse movement since the last event
    QVector2D pos = QVector2D(e->position());
    QVector2D diff = pos - lastMousePosition;
    lastMousePosition = pos;

    if (diff.isNull())
        return;

    // Shift + left button moves the camera instead of rotating the object
    if (e->modifiers() & Qt::ShiftModifier) {
        panCamera(diff);
        update();
        return;
    }

    // Turntable orbit around cameraTarget. Horizontal movement turns about the world
    // y axis, vertical movement turns about the camera x axis, so the horizon never tilts.
    // The camera moves opposite to the mouse so that the scene follows the mouse.
    const float degreesPerPixel = 0.5f;
    cameraYaw -= diff.x() * degreesPerPixel;
    cameraPitch = qBound(-g_maxPitch, cameraPitch + diff.y() * degreesPerPixel, g_maxPitch);
    updateCameraPosition();

    // Request an update
    update();
}

void MainWidget::wheelEvent(QWheelEvent *e)
{
    // One wheel notch is 120 units. Scaling the distance instead of adding to it
    // keeps each notch feeling the same at any distance.
    const float notches = e->angleDelta().y() / 120.0f;
    cameraDistance = qBound(g_minDistance, cameraDistance * std::pow(0.9f, notches), g_maxDistance);
    updateCameraPosition();

    // Request an update
    update();
}

// Spherical coordinates around the target: the eye sits at cameraDistance from
// cameraTarget, in the direction given by yaw and pitch
void MainWidget::updateCameraPosition()
{
    const float yaw = qDegreesToRadians(cameraYaw);
    const float pitch = qDegreesToRadians(cameraPitch);

    const QVector3D direction(std::sin(yaw) * std::cos(pitch),
                              std::sin(pitch),
                              std::cos(yaw) * std::cos(pitch));

    cameraPosition = cameraTarget + direction * cameraDistance;

    // Derivatives of direction with respect to yaw and pitch. Unlike a fixed world up
    // vector, these are never parallel to the view direction, even when looking straight
    // down, where lookAt would otherwise be undefined.
    cameraRight = QVector3D(std::cos(yaw), 0.0f, -std::sin(yaw));
    cameraUp = QVector3D(-std::sin(yaw) * std::sin(pitch),
                         std::cos(pitch),
                         -std::cos(yaw) * std::sin(pitch));

    emit cameraChanged(cameraRight, cameraUp);
}

// Translate the camera in its own image plane. Only the target moves here and the eye
// follows it, so the viewing direction does not change and the orbit center moves too.
void MainWidget::panCamera(const QVector2D &diff)
{
    // World size of one pixel on the plane through the target facing the camera:
    // the visible height at distance d is 2 * d * tan(fov / 2)
    const float visibleHeight = 2.0f * cameraDistance * std::tan(qDegreesToRadians(g_fovY) / 2.0f);
    const float worldPerPixel = visibleHeight / qMax(height(), 1);

    // The camera moves opposite to the mouse so that the scene follows the mouse.
    // Mouse y grows downwards, so a positive diff.y() moves the camera up.
    cameraTarget += (cameraUp * diff.y() - cameraRight * diff.x()) * worldPerPixel;
    updateCameraPosition();
}
//! [0]

void MainWidget::initializeGL()
{
    if (!initializeOpenGLFunctions())
        qFatal("Failed to initialize OpenGL 3.3 Core functions");

    const QSurfaceFormat actualFormat = context()->format();
    qInfo() << "OpenGL context:" << actualFormat.majorVersion()
            << actualFormat.minorVersion() << actualFormat.profile();

    glClearColor(0.23f, 0.23f, 0.23f, 1.0f);

    // Receive mouse moves without a pressed button, for the handle highlight
    setMouseTracking(true);

    initShaders();
    initTextures();

    geometries = new GeometryEngine;

    initScene();
}

// Initial object and camera poses in the world frame
void MainWidget::initScene()
{
    // Object frame: at the world origin, rotated -90 degrees about the world y axis
    objectPosition    = QVector3D(0.0f, 0.0f, 0.0f);
    objectOrientation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, -90.0f);

    // Camera: at (0, 0, 5) looking towards the origin, world y axis is up
    cameraTarget   = QVector3D(0.0f, 0.0f, 0.0f);
    cameraYaw      = 0.0f;
    cameraPitch    = 0.0f;
    cameraDistance = 5.0f;
    updateCameraPosition();
}

//! [3]
void MainWidget::initShaders()
{
    initShaderProgram(program, ":/vshader.glsl", ":/fshader.glsl");
    initShaderProgram(gridProgram, ":/gridvshader.glsl", ":/gridfshader.glsl");
}

void MainWidget::initShaderProgram(QOpenGLShaderProgram &shaderProgram, const QString &vertexShaderFile,
                                   const QString &fragmentShaderFile)
{
    // Compile vertex shader
    if (!shaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderFile))
        qFatal("Vertex shader compilation failed:\n%s", qPrintable(shaderProgram.log()));

    // Compile fragment shader
    if (!shaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShaderFile))
        qFatal("Fragment shader compilation failed:\n%s", qPrintable(shaderProgram.log()));

    // Link shader pipeline
    if (!shaderProgram.link())
        qFatal("Shader program link failed:\n%s", qPrintable(shaderProgram.log()));

    // Bind shader pipeline for use
    if (!shaderProgram.bind())
        qFatal("Shader program bind failed:\n%s", qPrintable(shaderProgram.log()));
}
//! [3]

//! [4]
void MainWidget::initTextures()
{
    // Load cube.png image
    texture = new QOpenGLTexture(QImage(":/cube.png").flipped());

    // Set nearest filtering mode for texture minification
    texture->setMinificationFilter(QOpenGLTexture::Nearest);

    // Set bilinear filtering mode for texture magnification
    texture->setMagnificationFilter(QOpenGLTexture::Linear);

    // Wrap texture coordinates by repeating
    // f.ex. texture coordinate (1.1, 1.2) is same as (0.1, 0.2)
    texture->setWrapMode(QOpenGLTexture::Repeat);
}
//! [4]

//! [5]
void MainWidget::resizeGL(int w, int h)
{
    // Keep track of the current window size
    g_windowWidth = w;
    g_windowHeight = h;

    // Calculate aspect ratio
    qreal aspect = qreal(g_windowWidth) / qreal(g_windowHeight ? g_windowHeight : 1);

    // Near and far planes are wide enough for every zoom level, field of view 45 degrees
    const qreal zNear = 0.1, zFar = 1000.0, fov = g_fovY;

    // Reset projection
    projection.setToIdentity();

    // Set perspective projection
    projection.perspective(fov, aspect, zNear, zFar);
}
//! [5]

void MainWidget::paintGL()
{
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//! [2]
    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);

    // Wireframe draws triangle edges only and shows the back faces too,
    // the other modes fill triangles with back face culling
    if (renderMode == RenderMode::Wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE);
    }
//! [2]

    texture->bind();
    program.bind();

//! [6]
    const QMatrix4x4 modelmatrix = objectMatrix();

    const QMatrix4x4 viewmatrix = viewMatrix();

    // Set modelview matrix as the combination of camera and object transformations
    QMatrix4x4 modelviewmatrix = viewmatrix * modelmatrix;

    // Set modelview-projection matrix
    program.setUniformValue("mvp_matrix", projection * modelviewmatrix);

    // Normals go from the object frame to the view frame with the inverse transpose
    program.setUniformValue("normal_matrix", modelviewmatrix.normalMatrix());
//! [6]

    // Use texture unit 0 which contains cube.png
    program.setUniformValue("cube_texture", 0);

    // Select the shading branch in the fragment shader
    program.setUniformValue("render_mode", static_cast<int>(renderMode));

    // Draw cube geometry
    geometries->drawCubeGeometry(&program);

    // Do not leak line mode into Qt's own use of this context
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Draw the floor grid, its vertices are already in the world frame
    gridProgram.bind();
    gridProgram.setUniformValue("mvp_matrix", projection * viewmatrix);
    geometries->drawGridGeometry(&gridProgram);

    // Draw the handles last and without depth test, so that the cube cannot hide them
    updateGizmoPlacement();
    if (objectSelected && gizmo.isVisible()) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        geometries->drawGizmoGeometry(&gridProgram, gizmo.buildVertices());
    }
}

// Model transformation: translation followed by rotation, Mo = To x Ro
// model trnsformation called world transformation in other texts
QMatrix4x4 MainWidget::objectMatrix() const
{
    QMatrix4x4 matrix;
    matrix.translate(objectPosition);
    matrix.rotate(objectOrientation);
    return matrix;
}

// View matrix from camera eye, target and up vector (same as gluLookAt / glm::lookAt)
QMatrix4x4 MainWidget::viewMatrix() const
{
    QMatrix4x4 matrix;
    matrix.lookAt(cameraPosition, cameraTarget, cameraUp);
    return matrix;
}
