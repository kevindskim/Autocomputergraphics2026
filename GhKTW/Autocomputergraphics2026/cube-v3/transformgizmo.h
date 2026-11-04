#ifndef TRANSFORMGIZMO_H
#define TRANSFORMGIZMO_H

#include "geometryengine.h"

#include <QMatrix4x4>
#include <QPointF>
#include <QQuaternion>
#include <QSize>
#include <QVector3D>

#include <utility>
#include <vector>

// Handles drawn at the selected object for moving and rotating it along the world axes.
// This class only does the math; the OpenGL drawing is in GeometryEngine.
class TransformGizmo
{
public:
    enum class Mode
    {
        Move,
        Rotate
    };

    // The value modulo 3 is the axis: the axis itself, the normal of the plane,
    // or the rotation axis in Rotate mode
    enum class Handle
    {
        None = -1,
        AxisX = 0,
        AxisY,
        AxisZ,
        PlaneYZ,
        PlaneZX,
        PlaneXY
    };

    struct View
    {
        QMatrix4x4 viewProjection;
        QVector3D eye;
        QVector3D right;
        QVector3D up;
        QVector3D forward;
        float tanHalfFov = 1.0f;
        QSize viewport;
    };

    struct Ray
    {
        QVector3D origin;
        QVector3D direction;    // Unit length
    };

    void setMode(Mode newMode);
    void setPlacement(const View &newView, const QVector3D &newCenter);
    void setHovered(Handle handle);

    // False when the object is behind the camera
    bool isVisible() const;

    // World space ray through a widget position
    Ray mouseRay(const QPointF &position) const;

    Handle handleAt(const QPointF &position) const;

    void beginDrag(Handle handle, const QPointF &position,
                   const QVector3D &objectPosition, const QQuaternion &objectOrientation);
    void drag(const QPointF &position, QVector3D *objectPosition, QQuaternion *objectOrientation) const;
    void endDrag();
    bool isDragging() const;

    // Triangles in the world frame
    std::vector<ColorVertex> buildVertices() const;

private:
    using Segment = std::pair<QVector3D, QVector3D>;

    float worldPerPixel() const;
    float length() const;
    QVector3D toCamera() const;
    QPointF project(const QVector3D &point) const;

    bool isAxisUsable(int axis) const;
    bool isPlaneUsable(int axis) const;
    std::vector<Segment> ringSegments(int axis) const;
    QVector3D planeHandleCorner(int axis, float u, float v) const;
    QVector3D colorOf(Handle handle) const;

    Handle moveHandleAt(const QPointF &position) const;
    Handle rotateHandleAt(const QPointF &position) const;
    bool closestParameterOnAxis(const Ray &ray, int axis, float *parameter) const;

    Mode mode = Mode::Move;
    View view;
    QVector3D center;
    Handle hovered = Handle::None;

    // State of the current drag, taken when it starts
    Handle dragHandle = Handle::None;
    QPointF dragStartMouse;
    QVector3D dragStartPosition;
    QQuaternion dragStartOrientation;
    QVector3D dragStartHit;         // Point grabbed on the plane
    float dragStartParameter = 0.0f;
    bool dragUsesRingPlane = false;
    QPointF dragScreenDirection;    // Used when the ring is seen edge on
};

#endif // TRANSFORMGIZMO_H
