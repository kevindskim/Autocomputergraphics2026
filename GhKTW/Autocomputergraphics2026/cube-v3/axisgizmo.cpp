#include "axisgizmo.h"

#include <QLineF>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

static const float g_handleRadius = 12.0f;

AxisGizmo::AxisGizmo(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(160, 160);

    // Receive mouse moves without a pressed button, for the hover highlight
    setMouseTracking(true);

    const QColor red(230, 80, 90);
    const QColor green(140, 195, 65);
    const QColor blue(65, 140, 230);

    handles = {
        {QVector3D( 1.0f,  0.0f,  0.0f), red,   "X"},
        {QVector3D(-1.0f,  0.0f,  0.0f), red,   ""},
        {QVector3D( 0.0f,  1.0f,  0.0f), green, "Y"},
        {QVector3D( 0.0f, -1.0f,  0.0f), green, ""},
        {QVector3D( 0.0f,  0.0f,  1.0f), blue,  "Z"},
        {QVector3D( 0.0f,  0.0f, -1.0f), blue,  ""}
    };
}

void AxisGizmo::setCameraAxes(const QVector3D &right, const QVector3D &up)
{
    cameraRight = right;
    cameraUp = up;
    update();
}

// Orthographic projection of a direction onto the camera image plane.
// Widget y grows downwards, so the up component is negated.
QPointF AxisGizmo::handlePosition(const QVector3D &direction) const
{
    const QPointF center = QRectF(rect()).center();
    const float length = qMin(width(), height()) / 2.0f - g_handleRadius - 2.0f;

    return center + QPointF(QVector3D::dotProduct(direction, cameraRight),
                            -QVector3D::dotProduct(direction, cameraUp)) * length;
}

// Positive when the direction points towards the camera
float AxisGizmo::depth(const QVector3D &direction) const
{
    const QVector3D towardCamera = QVector3D::crossProduct(cameraRight, cameraUp);
    return QVector3D::dotProduct(direction, towardCamera);
}

// Painter's algorithm: what is closer to the camera is drawn later, on top
QList<AxisGizmo::Handle> AxisGizmo::handlesBackToFront() const
{
    QList<Handle> sorted = handles;
    std::sort(sorted.begin(), sorted.end(), [this](const Handle &a, const Handle &b) {
        return depth(a.direction) < depth(b.direction);
    });
    return sorted;
}

// The front most handle under the position, or a null vector
QVector3D AxisGizmo::directionAt(const QPointF &position) const
{
    const QList<Handle> sorted = handlesBackToFront();
    for (auto handle = sorted.crbegin(); handle != sorted.crend(); ++handle) {
        if (QLineF(position, handlePosition(handle->direction)).length() <= g_handleRadius)
            return handle->direction;
    }
    return QVector3D();
}

void AxisGizmo::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(59, 59, 59));
    painter.drawRoundedRect(rect(), 8, 8);

    QFont font = painter.font();
    font.setBold(true);
    painter.setFont(font);

    const QPointF center = QRectF(rect()).center();
    const QPointF radius(g_handleRadius, g_handleRadius);

    for (const Handle &handle : handlesBackToFront()) {
        const QPointF position = handlePosition(handle.direction);
        const QRectF circle(position - radius, position + radius);
        const bool positive = !handle.label.isEmpty();
        const QPen outline(handle.direction == hoveredDirection ? QColor(Qt::white) : handle.color, 2);

        if (positive) {
            painter.setPen(QPen(handle.color, 3));
            painter.drawLine(center, position);
        }

        QColor fill = handle.color;
        if (!positive)
            fill.setAlpha(70);
        painter.setBrush(fill);
        painter.setPen(outline);
        painter.drawEllipse(circle);

        if (positive) {
            painter.setPen(Qt::black);
            painter.drawText(circle, Qt::AlignCenter, handle.label);
        }
    }
}

void AxisGizmo::mouseMoveEvent(QMouseEvent *e)
{
    const QVector3D direction = directionAt(e->position());
    if (direction == hoveredDirection)
        return;

    hoveredDirection = direction;
    setCursor(direction.isNull() ? Qt::ArrowCursor : Qt::PointingHandCursor);
    update();
}

void AxisGizmo::leaveEvent(QEvent *)
{
    hoveredDirection = QVector3D();
    update();
}

void AxisGizmo::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;

    const QVector3D direction = directionAt(e->position());
    if (direction.isNull())
        return;

    // The opposite handle is hidden behind the one we already look along,
    // so clicking that one again flips to the other side
    const bool alreadyFacing = depth(direction) > 0.999f;
    emit axisClicked(alreadyFacing ? -direction : direction);
}
