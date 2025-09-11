#include "MMTScreenOverlay.hpp"

#include "MMTVirtualDesktop.hpp"
#include "MMTMonitorManager.hpp"
#include "MMTSettings.hpp"

#include <QtGui>
#include <QGraphicsPixmapItem>
#include <QGraphicsItemAnimation>
#include <QGraphicsWidget>
#include <QLayout>
#include <QLineEdit>
#include <QPropertyAnimation>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace MMT {

class Canvas : public QGraphicsScene
{
public:
    Canvas(ScreenOverlay* parent) : QGraphicsScene(parent)
    {
        _overlay = parent;
    }

protected:
    void drawForeground(QPainter* painter, const QRectF& rect) override
    {
        QGraphicsScene::drawForeground(painter, rect);

        if (_overlay->_lineStart.size() != _overlay->_lineEnd.size() || _overlay->_lineStart.size() != _overlay->_lineColor.size())
        {
            return;
        }

        for (int i = 0; i < _overlay->_lineStart.size(); i++)
        {
            painter->setPen(_overlay->_lineColor[i]);
            painter->drawLine(_overlay->_lineStart[i] + rect.topLeft(), _overlay->_lineEnd[i] + rect.topLeft());
        }

        if (_overlay->_desktopNameFadeTimer->isActive())
        {
            QString title = VirtualDesktop::instance()->getDesktopName(VirtualDesktop::instance()->currentDesktop());
            QFont font("Segoe UI", 25);
            QRect fontRect = QFontMetrics(font).boundingRect(title);

            //qreal dpiScaling = _overlay->screen()->logicalDotsPerInch() / 96.0;

            qreal dpiScaling = 1.0;

            QRectF titleRect(rect.center().x() - fontRect.width() * dpiScaling /2, rect.center().y() - fontRect.height()  * dpiScaling  / 2, fontRect.width()  * dpiScaling , fontRect.height() * dpiScaling );

            painter->setBrush(QColor(24,19,69, 200));
            painter->drawRoundedRect(titleRect.adjusted(-10 * dpiScaling,-10 * dpiScaling,10 * dpiScaling,10 * dpiScaling),5 * dpiScaling,5 * dpiScaling);

            painter->setPen(Qt::white);
            painter->setFont(font);
            painter->drawText(titleRect, Qt::AlignCenter, VirtualDesktop::instance()->getDesktopName(VirtualDesktop::instance()->currentDesktop()));
        }

    }
private:
    ScreenOverlay* _overlay;
};

ScreenOverlay::ScreenOverlay(Screen* screen, QWidget* parent) : QGraphicsView(parent)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setGeometry(QRect(screen->logicalCoordinateRect().x(), screen->logicalCoordinateRect().y(), screen->logicalCoordinateRect().width(), screen->logicalCoordinateRect().height()));//screen->geometry());

    //hide overlay icon on taskbar
    HWND hwnd = (HWND)winId();
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle &= ~WS_EX_APPWINDOW;
    exStyle |= WS_EX_TOOLWINDOW;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    _screen = screen;

    auto scene = new Canvas(this);
    setScene(scene);
    setEnabled(false);
    setStyleSheet("border-width: 0px; border-style: solid; background: transparent");
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _pixmapitem = new Pixmap();
    scene->addItem(_pixmapitem);

    _animation = new QPropertyAnimation(_pixmapitem, "pos", this);
    _animation->setDuration(Settings::instance()->animationDuration());
    _animation->setStartValue(QPointF(0, 0));
    _animation->setEndValue(QPointF(_screen->logicalCoordinateRect().width(), 0));
    _animation->setEasingCurve(Settings::instance()->easingCurve());

    QObject::connect(_animation, &QPropertyAnimation::finished, this, &ScreenOverlay::clearContents);

    _desktopNameFadeTimer = new QTimer(this);
    _desktopNameFadeTimer->setSingleShot(true);
    QObject::connect(_desktopNameFadeTimer, &QTimer::timeout, this, [this]() {this->update(); });

    auto makeTopMostTimer = new QTimer(this);
    makeTopMostTimer->setInterval(1000);
    makeTopMostTimer->setSingleShot(false);
    QObject::connect(makeTopMostTimer, &QTimer::timeout, this, &ScreenOverlay::makeTopMost);
    makeTopMostTimer->start();
}

ScreenOverlay::~ScreenOverlay()
{
}

void ScreenOverlay::makeTopMost()
{
    SetWindowPos((HWND)winId(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

Screen* ScreenOverlay::screen() const
{
    return _screen;
}

void ScreenOverlay::setupDesktopSwitchAnimation()
{
    if (Settings::instance()->animationEnabled())
    {
        _screenshot = _screen->captureScreenShot();
    }
}

void ScreenOverlay::drawLines(const QList<QPointF>& lineStart, const QList<QPointF>& lineEnd, const QList<QColor>& lineColor)
{
    if (lineStart.size() != lineEnd.size() || lineStart.size() != lineColor.size())
    {
        qDebug() << "ScreenOverlay::drawLines input error!";
        return;
    }
    _lineStart = lineStart;
    _lineEnd = lineEnd;
    _lineColor = lineColor;
    update();
}

void ScreenOverlay::drawGridLines(double lineGap, const QList<QColor>& colors)
{
    auto screens = MonitorManager::instance()->screens();
    QRectF screenUnion;
    for (auto& screen : screens)
    {
        screenUnion = screenUnion.united(screen->physicalRect());
    }

    QList<QPointF> lineStart;
    QList<QPointF> lineEnd;
    QList<QColor> lineColor;

    int horizontalLines = std::floor(screenUnion.height() / lineGap);
    int verticalLines = std::floor(screenUnion.width() / lineGap);

    for (int i = 0; i < horizontalLines; i++)
    {
        double currentCoordinate = screenUnion.top() + i * lineGap;
        double mappedCoorinate = (currentCoordinate - _screen->physicalRect().top()) / _screen->physicalRect().height() * _screen->logicalCoordinateRect().height();
        lineStart.append(QPointF(0, mappedCoorinate));
        lineEnd.append(QPointF(_screen->logicalCoordinateRect().width(), mappedCoorinate));
        lineColor.append(colors[i % colors.size()]);
    }

    for (int i = 0; i < verticalLines; i++)
    {
        double currentCoordinate = screenUnion.left() + i * lineGap;
        double mappedCoorinate = (currentCoordinate - _screen->physicalRect().left()) / _screen->physicalRect().width() * _screen->logicalCoordinateRect().width();
        lineStart.append(QPointF(mappedCoorinate, 0));
        lineEnd.append(QPointF(mappedCoorinate, _screen->logicalCoordinateRect().height()));
        lineColor.append(colors[i % colors.size()]);
    }

    drawLines(lineStart, lineEnd, lineColor);
}

void ScreenOverlay::animateDesktopSwitch(Direction direction)
{
    if (Settings::instance()->animationEnabled())
    {
        _animation->stop();
        _renderedFrames = 0;

        _animation->setDuration(Settings::instance()->animationDuration());
        _animation->setEasingCurve(Settings::instance()->easingCurve());

        if (direction == Direction::Left)
        {
            _animation->setEndValue(QPointF(_screen->logicalCoordinateRect().width(), 0));
        }
        else
        {
            _animation->setEndValue(QPointF(-_screen->logicalCoordinateRect().width(), 0));
        }
        _pixmapitem->setPixmap(_screenshot);
        _animation->start();
    }
    else
    {
        emit setupDone();
    }
}

void ScreenOverlay::clearContents()
{
    _pixmapitem->setPixmap(QPixmap());
    update();
}

void ScreenOverlay::showDesktopName()
{
    _desktopNameFadeTimer->start(1000);
    update();
}

void ScreenOverlay::paintEvent(QPaintEvent* event)
{    
    QGraphicsView::paintEvent(event);
    _renderedFrames++;
    if (_renderedFrames == 2)
    {
        emit setupDone();
    }
}

Pixmap::Pixmap(QGraphicsItem* parent) : QGraphicsPixmapItem(parent)
{
}

Pixmap::~Pixmap()
{

}

}

