#ifndef AXISGIZMO_H
#define AXISGIZMO_H

#include <QColor>
#include <QList>
#include <QVector3D>
#include <QWidget>

// Shows the world axes as seen by the camera. Clicking the end of an axis
// asks for a view from that direction.
class AxisGizmo : public QWidget
{
    Q_OBJECT

public:
    explicit AxisGizmo(QWidget *parent = nullptr);

    // Where the end of a world axis direction is drawn, in widget coordinates
    QPointF handlePosition(const QVector3D &direction) const;

public slots:
    void setCameraAxes(const QVector3D &right, const QVector3D &up);

signals:
    void axisClicked(const QVector3D &direction);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *e) override;

private:
    struct Handle
    {
        QVector3D direction;
        QColor color;
        QString label;      // Empty for the negative end of an axis
    };

    float depth(const QVector3D &direction) const;
    QList<Handle> handlesBackToFront() const;
    QVector3D directionAt(const QPointF &position) const;

    QList<Handle> handles;
    QVector3D hoveredDirection;     // Null vector when no handle is hovered

    // Camera axes in the world frame
    QVector3D cameraRight = QVector3D(1.0f, 0.0f, 0.0f);
    QVector3D cameraUp = QVector3D(0.0f, 1.0f, 0.0f);
};

#endif // AXISGIZMO_H
