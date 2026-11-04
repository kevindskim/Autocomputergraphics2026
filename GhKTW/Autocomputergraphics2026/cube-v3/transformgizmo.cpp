#include "transformgizmo.h"

#include <QLineF>
#include <QtMath>

#include <algorithm>
#include <cmath>

// Sizes in pixels, so the handles look the same at any zoom
static const float g_lengthPixels = 150.0f;
static const float g_lineHalfWidthPixels = 1.5f;
static const float g_arrowLengthPixels = 18.0f;
static const float g_arrowHalfWidthPixels = 7.0f;
static const float g_pickPixels = 8.0f;

// Right angle mark between two axes, as a fraction of the axis length
static const float g_planeHandleSize = 0.35f;

static const int g_ringSegments = 96;

// |dot(axis, toCamera)| limits
static const float g_axisHiddenAbove = 0.985f;  // The axis points at the camera
static const float g_planeHiddenBelow = 0.12f;  // The plane is seen edge on
static const float g_ringPlaneAbove = 0.3f;     // The ring is open enough to intersect

static const QVector3D g_axes[3] = {
    QVector3D(1.0f, 0.0f, 0.0f),
    QVector3D(0.0f, 1.0f, 0.0f),
    QVector3D(0.0f, 0.0f, 1.0f)
};

static const QVector3D g_axisColors[3] = {
    QVector3D(0.90f, 0.31f, 0.35f),
    QVector3D(0.55f, 0.76f, 0.25f),
    QVector3D(0.25f, 0.55f, 0.90f)
};

static int axisOf(TransformGizmo::Handle handle)
{
    return static_cast<int>(handle) % 3;
}

static bool isPlane(TransformGizmo::Handle handle)
{
    return static_cast<int>(handle) >= 3;
}

static TransformGizmo::Handle axisHandle(int axis)
{
    return static_cast<TransformGizmo::Handle>(axis);
}

static TransformGizmo::Handle planeHandle(int axis)
{
    return static_cast<TransformGizmo::Handle>(axis + 3);
}

static float distanceToSegment(const QPointF &point, const QPointF &a, const QPointF &b)
{
    const QPointF ab = b - a;
    const qreal lengthSquared = QPointF::dotProduct(ab, ab);
    const qreal t = lengthSquared > 0.0
        ? qBound(0.0, QPointF::dotProduct(point - a, ab) / lengthSquared, 1.0)
        : 0.0;
    return QLineF(point, a + ab * t).length();
}

// A convex polygon contains the point when it is on the same side of every edge
static bool isInsideConvex(const QPointF &point, const QPointF *corners, int count)
{
    int positive = 0;
    int negative = 0;
    for (int i = 0; i < count; ++i) {
        const QPointF edge = corners[(i + 1) % count] - corners[i];
        const QPointF toPoint = point - corners[i];
        const qreal cross = edge.x() * toPoint.y() - edge.y() * toPoint.x();
        (cross >= 0.0 ? positive : negative) += 1;
    }
    return positive == 0 || negative == 0;
}

static bool intersectPlane(const TransformGizmo::Ray &ray, const QVector3D &pointOnPlane,
                           const QVector3D &normal, QVector3D *hit)
{
    const float denominator = QVector3D::dotProduct(ray.direction, normal);
    if (qAbs(denominator) < 1e-4f)
        return false;

    const float t = QVector3D::dotProduct(pointOnPlane - ray.origin, normal) / denominator;
    if (t <= 0.0f)
        return false;

    *hit = ray.origin + ray.direction * t;
    return true;
}

static void addTriangle(std::vector<ColorVertex> &vertices, const QVector3D &a, const QVector3D &b,
                        const QVector3D &c, const QVector3D &color)
{
    vertices.push_back({a, color});
    vertices.push_back({b, color});
    vertices.push_back({c, color});
}

// A flat strip that always faces the camera, because GL_LINES cannot be made
// thicker in the core profile. The ends overlap a little to close the corners.
static void addStrip(std::vector<ColorVertex> &vertices, const QVector3D &a, const QVector3D &b,
                     const QVector3D &color, float halfWidth, const QVector3D &eye)
{
    const QVector3D direction = (b - a).normalized();
    const QVector3D side = QVector3D::crossProduct(direction, eye - (a + b) * 0.5f).normalized() * halfWidth;
    const QVector3D start = a - direction * halfWidth;
    const QVector3D end = b + direction * halfWidth;

    addTriangle(vertices, start - side, end - side, end + side, color);
    addTriangle(vertices, start - side, end + side, start + side, color);
}

void TransformGizmo::setMode(Mode newMode)
{
    mode = newMode;
    hovered = Handle::None;
    endDrag();
}

void TransformGizmo::setPlacement(const View &newView, const QVector3D &newCenter)
{
    view = newView;
    center = newCenter;
}

void TransformGizmo::setHovered(Handle handle)
{
    hovered = handle;
}

bool TransformGizmo::isVisible() const
{
    return QVector3D::dotProduct(center - view.eye, view.forward) > 0.1f;
}

// World size of one pixel at the depth of the center, as in MainWidget::panCamera()
float TransformGizmo::worldPerPixel() const
{
    const float depth = QVector3D::dotProduct(center - view.eye, view.forward);
    return 2.0f * depth * view.tanHalfFov / qMax(view.viewport.height(), 1);
}

float TransformGizmo::length() const
{
    return g_lengthPixels * worldPerPixel();
}

QVector3D TransformGizmo::toCamera() const
{
    return (view.eye - center).normalized();
}

QPointF TransformGizmo::project(const QVector3D &point) const
{
    const QVector3D ndc = view.viewProjection.map(point);
    return QPointF((ndc.x() + 1.0f) / 2.0f * view.viewport.width(),
                   (1.0f - ndc.y()) / 2.0f * view.viewport.height());
}

// Inverse of the projection: from pixels to a direction through the image plane
TransformGizmo::Ray TransformGizmo::mouseRay(const QPointF &position) const
{
    const float width = qMax(view.viewport.width(), 1);
    const float height = qMax(view.viewport.height(), 1);
    const float ndcX = 2.0f * position.x() / width - 1.0f;
    const float ndcY = 1.0f - 2.0f * position.y() / height;
    const float aspect = width / height;

    const QVector3D direction = view.forward
        + view.right * (ndcX * view.tanHalfFov * aspect)
        + view.up * (ndcY * view.tanHalfFov);

    return {view.eye, direction.normalized()};
}

bool TransformGizmo::isAxisUsable(int axis) const
{
    return qAbs(QVector3D::dotProduct(g_axes[axis], toCamera())) < g_axisHiddenAbove;
}

bool TransformGizmo::isPlaneUsable(int axis) const
{
    return qAbs(QVector3D::dotProduct(g_axes[axis], toCamera())) > g_planeHiddenBelow;
}

// The part of the ring on the camera side. The more the ring faces the camera the
// longer the arc gets: half a circle when seen edge on, the full circle when facing it.
std::vector<TransformGizmo::Segment> TransformGizmo::ringSegments(int axis) const
{
    const QVector3D u = g_axes[(axis + 1) % 3];
    const QVector3D v = g_axes[(axis + 2) % 3];
    const QVector3D camera = toCamera();
    const float radius = length();
    const float limit = -radius * qAbs(QVector3D::dotProduct(g_axes[axis], camera)) - 0.001f * radius;

    const auto pointAt = [&](int i) {
        const float angle = 2.0f * float(M_PI) * i / g_ringSegments;
        return (u * std::cos(angle) + v * std::sin(angle)) * radius;
    };

    std::vector<Segment> segments;
    for (int i = 0; i < g_ringSegments; ++i) {
        const QVector3D a = pointAt(i);
        const QVector3D b = pointAt(i + 1);
        if (QVector3D::dotProduct((a + b) * 0.5f, camera) >= limit)
            segments.emplace_back(center + a, center + b);
    }
    return segments;
}

// Point (u, v) in fractions of the plane handle size, in the plane normal to the axis
QVector3D TransformGizmo::planeHandleCorner(int axis, float u, float v) const
{
    const float size = g_planeHandleSize * length();
    return center + g_axes[(axis + 1) % 3] * (u * size) + g_axes[(axis + 2) % 3] * (v * size);
}

TransformGizmo::Handle TransformGizmo::handleAt(const QPointF &position) const
{
    if (!isVisible())
        return Handle::None;

    return mode == Mode::Move ? moveHandleAt(position) : rotateHandleAt(position);
}

TransformGizmo::Handle TransformGizmo::moveHandleAt(const QPointF &position) const
{
    // Axes first, because they run along the edges of the plane handles
    Handle best = Handle::None;
    float bestDistance = g_pickPixels;
    const QPointF origin = project(center);

    for (int axis = 0; axis < 3; ++axis) {
        if (!isAxisUsable(axis))
            continue;

        const QPointF tip = project(center + g_axes[axis] * length());
        const float distance = distanceToSegment(position, origin, tip);
        if (distance <= bestDistance) {
            best = axisHandle(axis);
            bestDistance = distance;
        }
    }
    if (best != Handle::None)
        return best;

    // Plane handles can overlap on screen. The one facing the camera most wins.
    float bestFacing = 0.0f;
    for (int axis = 0; axis < 3; ++axis) {
        if (!isPlaneUsable(axis))
            continue;

        const QPointF corners[4] = {
            project(planeHandleCorner(axis, 0.0f, 0.0f)),
            project(planeHandleCorner(axis, 1.0f, 0.0f)),
            project(planeHandleCorner(axis, 1.0f, 1.0f)),
            project(planeHandleCorner(axis, 0.0f, 1.0f))
        };
        const float facing = qAbs(QVector3D::dotProduct(g_axes[axis], toCamera()));
        if (facing > bestFacing && isInsideConvex(position, corners, 4)) {
            best = planeHandle(axis);
            bestFacing = facing;
        }
    }
    return best;
}

TransformGizmo::Handle TransformGizmo::rotateHandleAt(const QPointF &position) const
{
    Handle best = Handle::None;
    float bestDistance = g_pickPixels;

    for (int axis = 0; axis < 3; ++axis) {
        for (const Segment &segment : ringSegments(axis)) {
            const float distance = distanceToSegment(position, project(segment.first), project(segment.second));
            if (distance <= bestDistance) {
                best = axisHandle(axis);
                bestDistance = distance;
            }
        }
    }
    return best;
}

// Closest point between the ray and the axis line through the start position.
// The result is the signed distance along the axis.
bool TransformGizmo::closestParameterOnAxis(const Ray &ray, int axis, float *parameter) const
{
    const QVector3D a = g_axes[axis];
    const QVector3D w = dragStartPosition - ray.origin;
    const float b = QVector3D::dotProduct(a, ray.direction);
    const float denominator = 1.0f - b * b;

    // The ray is parallel to the axis
    if (denominator < 1e-4f)
        return false;

    *parameter = (b * QVector3D::dotProduct(ray.direction, w) - QVector3D::dotProduct(a, w)) / denominator;
    return true;
}

void TransformGizmo::beginDrag(Handle handle, const QPointF &position,
                               const QVector3D &objectPosition, const QQuaternion &objectOrientation)
{
    dragHandle = handle;
    dragStartMouse = position;
    dragStartPosition = objectPosition;
    dragStartOrientation = objectOrientation;
    dragStartHit = objectPosition;
    dragStartParameter = 0.0f;

    const Ray ray = mouseRay(position);
    const int axis = axisOf(handle);
    const QVector3D direction = g_axes[axis];

    if (mode == Mode::Move) {
        if (isPlane(handle))
            intersectPlane(ray, objectPosition, direction, &dragStartHit);
        else
            closestParameterOnAxis(ray, axis, &dragStartParameter);
        return;
    }

    dragUsesRingPlane = qAbs(QVector3D::dotProduct(direction, toCamera())) >= g_ringPlaneAbove;
    if (dragUsesRingPlane) {
        intersectPlane(ray, objectPosition, direction, &dragStartHit);
        return;
    }

    // Seen edge on, the ring is almost a line and its plane cannot be hit reliably. The point
    // of the ring closest to the camera moves along axis x toCamera, so that direction on
    // the screen is the one that increases the angle.
    const QVector3D tangent = QVector3D::crossProduct(direction, toCamera());
    const QPointF onScreen = project(center + tangent * length()) - project(center);
    dragScreenDirection = onScreen / qMax(QLineF(QPointF(), onScreen).length(), 1e-3);
}

void TransformGizmo::drag(const QPointF &position, QVector3D *objectPosition, QQuaternion *objectOrientation) const
{
    if (!isDragging())
        return;

    const Ray ray = mouseRay(position);
    const int axis = axisOf(dragHandle);
    const QVector3D direction = g_axes[axis];

    if (mode == Mode::Move) {
        if (isPlane(dragHandle)) {
            QVector3D hit;
            if (intersectPlane(ray, dragStartPosition, direction, &hit))
                *objectPosition = dragStartPosition + hit - dragStartHit;
        } else {
            float parameter = 0.0f;
            if (closestParameterOnAxis(ray, axis, &parameter))
                *objectPosition = dragStartPosition + direction * (parameter - dragStartParameter);
        }
        return;
    }

    float radians = 0.0f;
    if (dragUsesRingPlane) {
        QVector3D hit;
        if (!intersectPlane(ray, dragStartPosition, direction, &hit))
            return;

        // Signed angle between the start and the current point, seen around the axis
        const QVector3D from = dragStartHit - dragStartPosition;
        const QVector3D to = hit - dragStartPosition;
        radians = std::atan2(QVector3D::dotProduct(direction, QVector3D::crossProduct(from, to)),
                             QVector3D::dotProduct(from, to));
    } else {
        // Arc length: dragging by the ring radius turns by one radian
        radians = QPointF::dotProduct(position - dragStartMouse, dragScreenDirection) / g_lengthPixels;
    }

    // The new rotation is about a world axis, so it is pre-multiplied: R(world) x Ro
    *objectOrientation = QQuaternion::fromAxisAndAngle(direction, qRadiansToDegrees(radians))
        * dragStartOrientation;
}

void TransformGizmo::endDrag()
{
    dragHandle = Handle::None;
}

bool TransformGizmo::isDragging() const
{
    return dragHandle != Handle::None;
}

QVector3D TransformGizmo::colorOf(Handle handle) const
{
    const QVector3D color = g_axisColors[axisOf(handle)];
    const Handle highlighted = isDragging() ? dragHandle : hovered;
    return handle == highlighted ? color * 0.4f + QVector3D(1.0f, 1.0f, 1.0f) * 0.6f : color;
}

std::vector<ColorVertex> TransformGizmo::buildVertices() const
{
    std::vector<ColorVertex> vertices;
    const float pixel = worldPerPixel();
    const float lineHalfWidth = g_lineHalfWidthPixels * pixel;

    if (mode == Mode::Rotate) {
        for (int axis = 0; axis < 3; ++axis) {
            const QVector3D color = colorOf(axisHandle(axis));
            for (const Segment &segment : ringSegments(axis))
                addStrip(vertices, segment.first, segment.second, color, lineHalfWidth, view.eye);
        }
        return vertices;
    }

    for (int axis = 0; axis < 3; ++axis) {
        if (!isPlaneUsable(axis))
            continue;

        // Right angle mark: from one axis to the corner, then to the other axis
        const QVector3D color = colorOf(planeHandle(axis));
        const QVector3D corner = planeHandleCorner(axis, 1.0f, 1.0f);
        addStrip(vertices, planeHandleCorner(axis, 1.0f, 0.0f), corner, color, lineHalfWidth, view.eye);
        addStrip(vertices, corner, planeHandleCorner(axis, 0.0f, 1.0f), color, lineHalfWidth, view.eye);
    }

    for (int axis = 0; axis < 3; ++axis) {
        if (!isAxisUsable(axis))
            continue;

        const QVector3D color = colorOf(axisHandle(axis));
        const QVector3D tip = center + g_axes[axis] * length();
        const QVector3D arrowBase = tip - g_axes[axis] * (g_arrowLengthPixels * pixel);
        addStrip(vertices, center, arrowBase, color, lineHalfWidth, view.eye);

        const QVector3D side = QVector3D::crossProduct(g_axes[axis], view.eye - arrowBase).normalized()
            * (g_arrowHalfWidthPixels * pixel);
        addTriangle(vertices, arrowBase - side, tip, arrowBase + side, color);
    }

    return vertices;
}
