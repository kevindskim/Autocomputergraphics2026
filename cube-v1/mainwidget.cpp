// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwidget.h"

#include <QDebug>
#include <QMouseEvent>
#include <QOpenGLContext>

#include <cmath>

// Window size; initial values are used as the size of the window at startup
static int g_windowWidth = 512;
static int g_windowHeight = 512;

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

//! [0]
void MainWidget::mousePressEvent(QMouseEvent *e)
{
    // Save mouse press position
    lastMousePosition = QVector2D(e->position());
}

void MainWidget::mouseMoveEvent(QMouseEvent *e)
{
    // Only rotate while the left button is held down
    if (!(e->buttons() & Qt::LeftButton))
        return;

    // Mouse movement since the last event
    QVector2D pos = QVector2D(e->position());
    QVector2D diff = pos - lastMousePosition;
    lastMousePosition = pos;

    if (diff.isNull())
        return;

    // Convert Mouse movement to opengl view coordinates
    const double dx = diff.x();
    const double dy = -diff.y();
    // Rotation axis of view window is perpendicular to 
    // the mouse position difference vector

    QVector3D n_view = QVector3D(-dy, dx, 0.0).normalized();

    // Rotation angle is proportional to the length of the mouse movement
    const float degreesPerPixel = 0.5f;
    float angle = diff.length() * degreesPerPixel;

    // Update rotation
    rotation = QQuaternion::fromAxisAndAngle(n_view, angle) * rotation;

    // Request an update
    update();
}
//! [0]

void MainWidget::initializeGL()
{
    if (!initializeOpenGLFunctions())
        qFatal("Failed to initialize OpenGL 3.3 Core functions");

    const QSurfaceFormat actualFormat = context()->format();
    qInfo() << "OpenGL context:" << actualFormat.majorVersion()
            << actualFormat.minorVersion() << actualFormat.profile();

    glClearColor(0, 0, 0, 1);

    initShaders();
    initTextures();

    geometries = new GeometryEngine;
}

//! [3]
void MainWidget::initShaders()
{
    // Compile vertex shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl"))
        qFatal("Vertex shader compilation failed:\n%s", qPrintable(program.log()));

    // Compile fragment shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl"))
        qFatal("Fragment shader compilation failed:\n%s", qPrintable(program.log()));

    // Link shader pipeline
    if (!program.link())
        qFatal("Shader program link failed:\n%s", qPrintable(program.log()));

    // Bind shader pipeline for use
    if (!program.bind())
        qFatal("Shader program bind failed:\n%s", qPrintable(program.log()));
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

    // Set near plane to 3.0, far plane to 7.0, field of view 45 degrees
    const qreal zNear = 3.0, zFar = 7.0, fov = 45.0;

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

    // Enable back face culling
    glEnable(GL_CULL_FACE);
//! [2]

    texture->bind();
    program.bind();

//! [6]
    // Calculate model transformation, which includes translation and rotation of the cube
    // Model transformation: translation followed by rotation, Mo = To x Ro
    // model trnsformation called world transformation in other texts
    QMatrix4x4 modelmatrix;                 // Model matrix for cube transformations, Identity matrix initially
    modelmatrix.translate(0.0, 0.0, 0.0);   // Mo = To 
    modelmatrix.rotate(rotation);           // Mo = To x Ro

    // Set camera position and orientation (view transformation)
    QMatrix4x4 cameraposmatrix;
    cameraposmatrix.setToIdentity();  // Reset camera matrix to identity before applying transformations
    cameraposmatrix.translate(0.0, 0.0, 5.0);  // Camera is at (0, 0, 5) looking towards the origin

    // set view matrix as the camera transformation
    QMatrix4x4 viewmatrix = cameraposmatrix.inverted();

    // Set modelview matrix as the combination of camera and object transformations
    QMatrix4x4 modelviewmatrix = viewmatrix * modelmatrix;

    // Set modelview-projection matrix
    program.setUniformValue("mvp_matrix", projection * modelviewmatrix);
//! [6]

    // Use texture unit 0 which contains cube.png
    program.setUniformValue("cube_texture", 0);

    // Draw cube geometry
    geometries->drawCubeGeometry(&program);
}
