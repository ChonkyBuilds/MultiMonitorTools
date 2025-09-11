#pragma once
#include "MMT.hpp"

#include <QGraphicsView>
#include <QObject>
#include <QGraphicsPixmapItem>
#include <QList>
#include <QPointF>

class QColor;
class QPaintEvent;
class QPropertyAnimation;
class QPixmap;
class QTimer;

namespace MMT{

class Screen;
class Pixmap;

class ScreenOverlay : public QGraphicsView
{
    Q_OBJECT
public:
    ScreenOverlay(Screen* screen, QWidget* parent = 0);
    ~ScreenOverlay();

public:
    void makeTopMost();

public:
    Screen* screen() const;
    void setupDesktopSwitchAnimation();

public:
    void drawLines(const QList<QPointF>& lineStart, const QList<QPointF>& lineEnd, const QList<QColor>& lineColor);
    void drawGridLines(double lineGap, const QList<QColor>& colors);

public slots:
    void animateDesktopSwitch(Direction);
    void clearContents();
    void showDesktopName();

signals:
    void setupDone();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Screen* _screen = nullptr;
    int _renderedFrames = -1;
    Pixmap* _pixmapitem = nullptr;
    QPropertyAnimation* _animation = nullptr;
    QPixmap _screenshot;
    QList<QPointF> _lineStart;
    QList<QPointF> _lineEnd;
    QList<QColor> _lineColor;
    QTimer* _desktopNameFadeTimer;
    friend class Canvas;
};

class Pixmap : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
        Q_PROPERTY(QPointF pos READ pos WRITE setPos)
public:
    Pixmap(QGraphicsItem* parent = nullptr);
    ~Pixmap();
};

}