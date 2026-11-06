// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwidget.h"

#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPainter>
#include <QWheelEvent>
#include <QtMath>

#include <cmath>

// Window size; initial values are used as the size of the window at startup
static int g_windowWidth = 512;
static int g_windowHeight = 512;

MainWidget::MainWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    // Receive keyboard events
    setFocusPolicy(Qt::StrongFocus);

    // ~60 fps animation for showDiceFace()
    animationTimer.setInterval(16);
    connect(&animationTimer, &QTimer::timeout, this, &MainWidget::animationStep);

    // ~60 fps inertia after a mouse drag
    inertiaTimer.setInterval(16);
    connect(&inertiaTimer, &QTimer::timeout, this, &MainWidget::inertiaStep);

    initScene();
}

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
    delete numberTexture;
    delete geometries;
    doneCurrent();
}

//! [0]
void MainWidget::mousePressEvent(QMouseEvent *e)
{
    // Grabbing the cube stops any inertia
    stopInertia();
    dragClock.start();

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
    const float angle = diff.length() * degreesPerPixel;
    rotateObject(n_view, angle);

    // Remember the drag speed (smoothed) for the inertia after release
    const qint64 dt = qMax<qint64>(1, dragClock.restart());
    const float speed = qMin(angle * 1000.0f / float(dt), 1080.0f);
    inertiaSpeed = 0.6f * speed + 0.4f * inertiaSpeed;
    inertiaAxis = n_view;
}

void MainWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;

    // Spin on only if the mouse was still moving when released
    if (dragClock.isValid() && dragClock.elapsed() < 50 && inertiaSpeed > 20.0f)
        inertiaTimer.start();
    else
        stopInertia();
}

void MainWidget::inertiaStep()
{
    rotateObject(inertiaAxis, inertiaSpeed * inertiaTimer.interval() / 1000.0f);

    // Friction: lose 5% of the speed every frame
    inertiaSpeed *= 0.95f;
    if (inertiaSpeed < 5.0f)
        stopInertia();
}

void MainWidget::stopInertia()
{
    inertiaTimer.stop();
    inertiaSpeed = 0.0f;
}
//! [0]

// Same frame conversion as cube-v2, but computed from the current state
// so that it is also correct before the next paintGL() (keyboard, camera orbit)
void MainWidget::rotateObject(const QVector3D &n_view, float angle)
{
    animationTimer.stop();

    // view frame -> world frame : n_world = R_cam * n_view
    // (in the die view the "camera" is the die itself, so this is the die's view frame)
    QVector3D n_world = viewMatrix().inverted().mapVector(n_view);

    // Die view: turning the die turns its eye. Turn the other way so that it looks
    // where the mouse goes, like a first-person game (drag right -> look right).
    if (dieView)
        angle = -angle;

    if (cameraFollowsObject && !dieView) {
        // Camera mode: the cube and the floor stay still in the world, only the camera
        // orbits the cube. Orbiting by the inverse rotation makes the scene on screen
        // turn the same way as the drag.
        rotateCameraAboutObject(QQuaternion::fromAxisAndAngle(n_world.normalized(), -angle));
        update();
        return;
    }

    // world frame -> object frame : n_object = Ro^-1 * n_world
    QVector3D n_object = objectOrientation.inverted().rotatedVector(n_world).normalized();

    // Mo' = To x Ro x R(n_object)
    objectOrientation = (objectOrientation * QQuaternion::fromAxisAndAngle(n_object, angle)).normalized();

    update();
}

// Rotate the camera pose (eye, target, up) about the object origin by a world-frame rotation
void MainWidget::rotateCameraAboutObject(const QQuaternion &q)
{
    cameraPosition = objectPosition + q.rotatedVector(cameraPosition - objectPosition);
    cameraTarget   = objectPosition + q.rotatedVector(cameraTarget - objectPosition);
    cameraUp       = q.rotatedVector(cameraUp);
}

void MainWidget::setCameraFollowsObject(bool follows)
{
    // The current camera pose is kept; from now on it is fixed in the chosen frame
    cameraFollowsObject = follows;
    emit statusMessage(follows ? "카메라 이동 모드: 주사위와 바닥은 고정, 드래그하면 카메라가 주사위 주위를 돎"
                               : "주사위 회전 모드: 카메라는 고정, 드래그하면 주사위가 돎");
}

void MainWidget::wheelEvent(QWheelEvent *e)
{
    // Ctrl + wheel: one notch (120) changes the field of view by 5 degrees
    if (e->modifiers() & Qt::ControlModifier) {
        setFieldOfView(fieldOfView - 5.0f * e->angleDelta().y() / 120.0f);
        return;
    }

    // One wheel notch (120) moves the camera 0.5 towards / away from the target
    zoomCamera(-0.5f * e->angleDelta().y() / 120.0f);
}

void MainWidget::setFieldOfView(float degrees)
{
    // 15 degrees (telephoto) ~ 100 degrees (wide angle)
    fieldOfView = qBound(15.0f, degrees, 100.0f);
    updateProjection();
    update();
    emit statusMessage(QString("시야각: %1°%2").arg(fieldOfView, 0, 'f', 0)
                       .arg(perspectiveProjection ? "" : " (직교 투영에서는 원근 투영으로 바꿔야 보임)"));
    emit fieldOfViewChanged(fieldOfView);
}

void MainWidget::setShape(int shape)
{
    if (!geometries)
        return;

    // The VBOs belong to this widget's GL context
    makeCurrent();
    geometries->setShape(GeometryEngine::Shape(shape));
    doneCurrent();

    static const char *names[] = {"정육면체 주사위 (D6)", "정사면체 (D4)", "정팔면체 (D8)", "정이십면체 (D20)"};
    emit statusMessage(QString("도형: %1, 면 %2개").arg(names[qBound(0, shape, 3)]).arg(geometries->faceCount()));
    update();
}

void MainWidget::setDieView(bool enabled)
{
    if (dieView == enabled)
        return;
    dieView = enabled;
    stopInertia();
    update();
    emit dieViewChanged(enabled);
    emit statusMessage(enabled ? "주사위 시점: 1번 면의 눈으로 봄, 노란 모델이 카메라 (드래그로 둘러보기)"
                               : "카메라 시점");
}

void MainWidget::keyPressEvent(QKeyEvent *e)
{
    const float step = 5.0f;   // degrees per key press

    switch (e->key()) {
    // Arrow keys rotate the cube like a mouse drag in that direction
    case Qt::Key_Left:  rotateObject(QVector3D(0, -1, 0), step); break;
    case Qt::Key_Right: rotateObject(QVector3D(0,  1, 0), step); break;
    case Qt::Key_Up:    rotateObject(QVector3D(-1, 0, 0), step); break;
    case Qt::Key_Down:  rotateObject(QVector3D( 1, 0, 0), step); break;

    // A / D orbit the camera, W / S move it closer / farther
    case Qt::Key_A: orbitCamera(-step); break;
    case Qt::Key_D: orbitCamera( step); break;
    // Q / E orbit the camera up / down about its own x axis
    case Qt::Key_Q: pitchCamera( step); break;
    case Qt::Key_E: pitchCamera(-step); break;
    case Qt::Key_W: zoomCamera(-0.25f); break;
    case Qt::Key_S: zoomCamera( 0.25f); break;

    // Z / X widen / narrow the field of view (wide angle / telephoto)
    case Qt::Key_Z: setFieldOfView(fieldOfView + 5.0f); break;
    case Qt::Key_X: setFieldOfView(fieldOfView - 5.0f); break;

    // J / K move the light around the world y axis
    case Qt::Key_J: lightAngle -= 10.0f; update(); break;
    case Qt::Key_K: lightAngle += 10.0f; update(); break;

    // Number keys turn that face towards the camera. A second digit within 0.8 s
    // makes a two-digit number (1 then 2 -> face 12) for the D8 / D20
    case Qt::Key_0: case Qt::Key_1: case Qt::Key_2: case Qt::Key_3: case Qt::Key_4:
    case Qt::Key_5: case Qt::Key_6: case Qt::Key_7: case Qt::Key_8: case Qt::Key_9: {
        const int digit = e->key() - Qt::Key_0;
        const int twoDigits = typedNumber * 10 + digit;
        if (typedNumber > 0 && digitClock.isValid() && digitClock.elapsed() < 800 && twoDigits <= faceCount())
            typedNumber = twoDigits;
        else
            typedNumber = digit;
        digitClock.start();
        showDiceFace(typedNumber);
        break;
    }

    default:
        QOpenGLWidget::keyPressEvent(e);
    }
}

void MainWidget::orbitCamera(float degrees)
{
    // Rotate the eye about the world y axis through the target
    const QQuaternion q = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, degrees);
    cameraPosition = cameraTarget + q.rotatedVector(cameraPosition - cameraTarget);
    cameraUp = q.rotatedVector(cameraUp);   // the up vector may be tilted in object-frame mode
    update();
}

void MainWidget::pitchCamera(float degrees)
{
    // Rotate the eye about the camera x axis (right vector) through the target;
    // the up vector turns with it, so the camera can go over the top without flipping
    const QVector3D right = QVector3D::crossProduct(cameraTarget - cameraPosition, cameraUp).normalized();
    const QQuaternion q = QQuaternion::fromAxisAndAngle(right, degrees);
    cameraPosition = cameraTarget + q.rotatedVector(cameraPosition - cameraTarget);
    cameraUp = q.rotatedVector(cameraUp);
    update();
}

void MainWidget::zoomCamera(float delta)
{
    QVector3D dir = cameraPosition - cameraTarget;
    const float distance = qBound(3.0f, dir.length() + delta, 15.0f);
    cameraPosition = cameraTarget + dir.normalized() * distance;

    updateProjection();   // orthographic size follows the distance
    update();
    emit statusMessage(QString("카메라 거리: %1").arg(distance, 0, 'f', 2));
}

void MainWidget::showDiceFace(int number)
{
    // Outward normal (object frame) of the face that shows this number, from the current shape
    if (!geometries || number < 1 || number > geometries->faceCount())
        return;

    // Shortest rotation (in the world frame) from where the face points now to the camera
    const QVector3D current = objectOrientation.rotatedVector(geometries->faceNormal(number));
    const QVector3D toCamera = (cameraPosition - cameraTarget).normalized();

    const QQuaternion turn = QQuaternion::rotationTo(current, toCamera);

    stopInertia();
    animateCamera = cameraFollowsObject;
    if (animateCamera) {
        // The cube is fixed to the camera, so move the camera to the face instead (inverse rotation)
        animationCamRot = turn.inverted();
        animationCamPos = cameraPosition;
        animationCamUp  = cameraUp;
    } else {
        animationFrom = objectOrientation;
        animationTo = (turn * objectOrientation).normalized();
    }
    animationT = 0.0f;
    animationTimer.start();

    emit statusMessage(QString("주사위 %1 면을 카메라 쪽으로 회전").arg(number));
}

void MainWidget::animationStep()
{
    // 20 steps of 16 ms, about 0.3 s
    animationT = qMin(1.0f, animationT + 0.05f);
    if (animateCamera) {
        const QQuaternion q = QQuaternion::slerp(QQuaternion(), animationCamRot, animationT);
        cameraPosition = objectPosition + q.rotatedVector(animationCamPos - objectPosition);
        cameraUp = q.rotatedVector(animationCamUp);
    } else {
        objectOrientation = QQuaternion::slerp(animationFrom, animationTo, animationT);
    }
    if (animationT >= 1.0f)
        animationTimer.stop();
    update();
}

void MainWidget::resetScene()
{
    animationTimer.stop();
    stopInertia();
    initScene();
    updateProjection();
    update();
    emit statusMessage("초기 상태로 되돌림");
}

void MainWidget::setLightingEnabled(bool enabled)
{
    lightingEnabled = enabled;
    update();
    emit statusMessage(enabled ? "조명 켜짐 (Phong)" : "조명 꺼짐 (텍스처만)");
}

void MainWidget::setLightColor(const QVector3D &color)
{
    lightColor = color;
    update();
}

void MainWidget::setPerspectiveProjection(bool perspective)
{
    perspectiveProjection = perspective;
    updateProjection();
    update();
    emit statusMessage(perspective ? "원근 투영" : "직교 투영");
}

void MainWidget::initializeGL()
{
    if (!initializeOpenGLFunctions())
        qFatal("Failed to initialize OpenGL 3.3 Core functions");

    const QSurfaceFormat actualFormat = context()->format();
    qInfo() << "OpenGL context:" << actualFormat.majorVersion()
            << actualFormat.minorVersion() << actualFormat.profile();

    // Dark slate instead of black so the scene has some depth
    glClearColor(0.10f, 0.11f, 0.14f, 1.0f);

    initShaders();
    initTextures();

    geometries = new GeometryEngine;
}

// Initial object and camera poses in the world frame
void MainWidget::initScene()
{
    // Object frame: at the world origin, rotated -90 degrees about the world y axis
    objectPosition    = QVector3D(0.0f, 0.0f, 0.0f);
    objectOrientation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, -90.0f);

    // Camera: at (0, 0, 5) looking towards the origin, world y axis is up
    cameraPosition = QVector3D(0.0f, 0.0f, 5.0f);
    cameraTarget   = QVector3D(0.0f, 0.0f, 0.0f);
    cameraUp       = QVector3D(0.0f, 1.0f, 0.0f);

    lightAngle = 45.0f;
    fieldOfView = 45.0f;
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

    // cube-final: simple per-vertex color shader for the grid and axes
    static const char *lineVertexShader =
        "#version 330 core\n"
        "in vec3 a_position;\n"
        "in vec3 a_color;\n"
        "uniform mat4 mvp_matrix;\n"
        "out vec3 v_color;\n"
        "void main() {\n"
        "    gl_Position = mvp_matrix * vec4(a_position, 1.0);\n"
        "    v_color = a_color;\n"
        "}\n";
    static const char *lineFragmentShader =
        "#version 330 core\n"
        "in vec3 v_color;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    fragColor = vec4(v_color, 1.0);\n"
        "}\n";
    if (!lineProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, lineVertexShader)
        || !lineProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, lineFragmentShader)
        || !lineProgram.link())
        qFatal("Line shader failed:\n%s", qPrintable(lineProgram.log()));
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

    // cube-final: number texture for the polyhedra, drawn here with QPainter.
    // One cell per number (5 x 4 cells); the triangle in each cell is the one
    // GeometryEngine::initPolyhedron() maps onto the face.
    const int cols = GeometryEngine::AtlasColumns, rows = GeometryEngine::AtlasRows, cell = 256;
    QImage atlas(cols * cell, rows * cell, QImage::Format_RGBA8888);
    atlas.fill(QColor(250, 246, 236));

    QPainter painter(&atlas);
    painter.setRenderHint(QPainter::Antialiasing);
    QFont font("Arial");
    font.setPixelSize(int(cell * 0.26));
    font.setBold(true);
    painter.setFont(font);

    const QColor ink(150, 30, 40);
    for (int i = 0; i < cols * rows; ++i) {
        const QRectF r((i % cols) * cell, (i / cols) * cell, cell, cell);
        // Texture coordinate (u, v) inside the cell -> image pixel (v goes up, image y goes down)
        auto px = [&r, cell](float u, float v) { return QPointF(r.left() + u * cell, r.bottom() - v * cell); };

        painter.setPen(QPen(ink, cell * 0.03));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(QPolygonF({px(0.1f, 0.12f), px(0.9f, 0.12f), px(0.5f, 0.92f)}));

        // Number at the centroid of the triangle; 6 and 9 get a dot so they can be told apart
        const int number = i + 1;
        const QString text = QString::number(number) + ((number == 6 || number == 9) ? "." : "");
        const QPointF c = px(0.5f, (0.12f + 0.12f + 0.92f) / 3.0f);
        painter.drawText(QRectF(c.x() - cell * 0.3, c.y() - cell * 0.17, cell * 0.6, cell * 0.34),
                         Qt::AlignCenter, text);
    }
    painter.end();

    // Flipped like cube.png so that t = 0 is the bottom of the image
    numberTexture = new QOpenGLTexture(atlas.flipped());
    numberTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    numberTexture->setMagnificationFilter(QOpenGLTexture::Linear);
}
//! [4]

//! [5]
void MainWidget::resizeGL(int w, int h)
{
    // Keep track of the current window size
    g_windowWidth = w;
    g_windowHeight = h;

    updateProjection();
}
//! [5]

void MainWidget::updateProjection()
{
    // Calculate aspect ratio
    const float aspect = float(g_windowWidth) / float(g_windowHeight ? g_windowHeight : 1);

    // Near / far are wider than cube-v2 (3, 7) so the camera can zoom from 3 to 15
    const float zNear = 0.5f, zFar = 50.0f;

    projection.setToIdentity();

    if (perspectiveProjection) {
        projection.perspective(fieldOfView, aspect, zNear, zFar);
    } else {
        // Same visible height at the target distance as a standard 45 degree perspective view
        // (an orthographic projection has no field of view)
        const float halfH = (cameraPosition - cameraTarget).length() * std::tan(qDegreesToRadians(45.0f / 2.0f));
        projection.ortho(-halfH * aspect, halfH * aspect, -halfH, halfH, zNear, zFar);
    }
}

QMatrix4x4 MainWidget::cameraViewMatrix() const
{
    // View matrix from camera eye, target and up vector (same as gluLookAt / glm::lookAt)
    QMatrix4x4 view;
    view.lookAt(cameraPosition, cameraTarget, cameraUp);
    return view;
}

QMatrix4x4 MainWidget::viewMatrix() const
{
    if (!dieView || !geometries)
        return cameraViewMatrix();

    // Die view: the eye sits just outside face 1 and looks along its normal.
    // Forward and up are object-frame directions turned into the world frame by Ro.
    const QVector3D n1 = geometries->faceNormal(1);
    const QVector3D forward = objectOrientation.rotatedVector(n1).normalized();
    const QVector3D upLocal = std::abs(n1.y()) < 0.9f ? QVector3D(0, 1, 0) : QVector3D(0, 0, 1);
    QVector3D up = objectOrientation.rotatedVector(upLocal);
    up = (up - QVector3D::dotProduct(up, forward) * forward).normalized();

    const QVector3D eye = objectPosition + forward * (geometries->radius() + 0.05f);

    QMatrix4x4 view;
    view.lookAt(eye, eye + forward, up);
    return view;
}

QVector3D MainWidget::lightPosition() const
{
    // Light circles the world y axis at radius 4, height 3
    const float rad = qDegreesToRadians(lightAngle);
    return QVector3D(4.0f * std::sin(rad), 3.0f, 4.0f * std::cos(rad));
}

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

    // The dice photo for the cube, the generated numbers for the other shapes
    if (geometries->shape() == GeometryEngine::Shape::Cube)
        texture->bind();
    else
        numberTexture->bind();
    program.bind();

//! [6]
    // Model transformation: translation followed by rotation, Mo = To x Ro
    modelmatrix.setToIdentity();
    modelmatrix.translate(objectPosition);
    modelmatrix.rotate(objectOrientation);

    QMatrix4x4 viewmatrix = viewMatrix();

    // Camera pose in the world frame (view frame -> world frame) is the inverse of the view matrix
    cameraposmatrix = viewmatrix.inverted();

    // Set modelview-projection matrix
    program.setUniformValue("mvp_matrix", projection * viewmatrix * modelmatrix);
//! [6]

    // cube-final: lighting uniforms (world frame)
    program.setUniformValue("model_matrix", modelmatrix);
    program.setUniformValue("normal_matrix", modelmatrix.normalMatrix());
    program.setUniformValue("lighting_enabled", lightingEnabled ? 1 : 0);
    program.setUniformValue("light_pos", lightPosition());
    program.setUniformValue("light_color", lightColor);
    // Eye position = origin of the view frame mapped to the world (camera or die eye)
    program.setUniformValue("view_pos", cameraposmatrix.map(QVector3D(0.0f, 0.0f, 0.0f)));

    // Use texture unit 0 which contains cube.png
    program.setUniformValue("cube_texture", 0);

    // Draw cube geometry
    geometries->drawCubeGeometry(&program);

    // cube-final: reference lines, the grid / world axes use the world frame (no model matrix),
    // the object axes use the object frame (with the model matrix)
    lineProgram.bind();
    lineProgram.setUniformValue("mvp_matrix", projection * viewmatrix);
    geometries->drawWorldLines(&lineProgram);
    if (!dieView) {
        lineProgram.setUniformValue("mvp_matrix", projection * viewmatrix * modelmatrix);
        geometries->drawObjectAxes(&lineProgram);
    } else {
        // Die view: draw the real camera. Its model is defined in the camera frame,
        // so the model matrix is the camera pose (camera frame -> world frame) = V_cam^-1
        lineProgram.setUniformValue("mvp_matrix", projection * viewmatrix * cameraViewMatrix().inverted());
        geometries->drawCameraModel(&lineProgram);
    }
}
