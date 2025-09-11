#pragma once
#include <QGraphicsView>

class QRectF;
class QWheelEvent;
class QMouseEvent;

namespace MMT
{
class Screen;
class MonitorLayoutEditor : public QGraphicsView
{
    Q_OBJECT

public:
    MonitorLayoutEditor(QWidget* parent = nullptr);

public:
    void addScreen(const Screen*);
    void clearScreens();
    void alignScreens();

public:
    const Screen* selectedScreen();
    void tryModifyScreenGeometry(const Screen* screen, QRectF& geometry);

signals:
    void screenGeometryChanged(const Screen*, const QRectF& geometry);
    void screenSelectionChanged(const Screen*);

public:
    void zoom(double factor);
    void zoomToFit();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};

}