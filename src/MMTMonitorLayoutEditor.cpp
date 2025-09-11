#include "MMTMonitorLayoutEditor.hpp"
#include "MMTMonitorManager.hpp"

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QMessageBox>
#include <QLayout>
#include <QPushButton>

namespace MMT
{


namespace
{

struct Interval
{
    Interval(double s, double e)
    {
        start = s;
        end = e;
    }
    double start;
    double end;
};

bool intersects(const Interval& interval1, const Interval& interval2)
{
    if (interval1.start >= interval2.start && interval1.start <= interval2.end) return true;
    if (interval1.end >= interval2.start && interval1.end <= interval2.end) return true;
    if (interval2.start >= interval1.start && interval2.start <= interval1.end) return true;
    if (interval2.end >= interval1.start && interval2.end <= interval1.end) return true;
    return false;
}

typedef std::vector<Interval> Intervals;

bool intersects(const QRectF& rect1, const QRectF& rect2)
{
    if (rect1.left() >= rect2.right() || rect1.right() <= rect2.left() || rect1.top() >= rect2.bottom() || rect1.bottom() <= rect2.top())
    {
        return false;
    }
    return true;
}

std::vector<Interval> subtract(const std::vector<Interval>& intervals, const Interval& sub)
{
    std::vector<Interval> result;
    for (auto& interval : intervals)
    {
        if (sub.end <= interval.start || sub.start >= interval.end)
        {
            result.push_back(interval);
        }
        else
        {
            if (sub.start >= interval.start && sub.end <= interval.end)
            {
                result.push_back(Interval(interval.start, sub.start));
                result.push_back(Interval(sub.end, interval.end));
            }
            else if (sub.start < interval.start && sub.end <= interval.end)
            {
                result.push_back(Interval(sub.end, interval.end));
            }
            else if (sub.start >= interval.start && sub.end > interval.end)
            {
                result.push_back(Interval(interval.start, sub.start));
            }
        }
    }
    return result;
}

//recall that the coordinate system is mirrored on the y-axis
QRectF quarterTurnClockwise(const QRectF& rect)
{
    return QRectF(QPointF(-rect.bottom(), rect.left()), QPointF(-rect.top(), rect.right()));
}

QPointF quarterTurnCounterClockwise(const QPointF& point)
{
    return QPointF(point.y(), -point.x());
}

QPointF findBestLeftDisplacement(QRectF& rectToAlign, const QList<QRectF>& existingRects)
{
    double currentBestDisplacement = std::numeric_limits<double>::max();
    QPointF bestDisplacement(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());

    for (auto& targetRect : existingRects)
    {
        QPointF horizontalDisplacement = QPointF(targetRect.right() - rectToAlign.left(), 0);

        std::vector<Interval> collisionFreeZones;
        collisionFreeZones.push_back(Interval(targetRect.top() - rectToAlign.height(), targetRect.bottom()));

        for (auto& testCollisionRect : existingRects)
        {
            if (testCollisionRect == targetRect) continue;
            if (!intersects(QRectF(targetRect.right(), targetRect.top() - rectToAlign.height(), rectToAlign.width(), targetRect.height() + 2 * rectToAlign.height()), testCollisionRect)) continue;

            Interval collisionZone(testCollisionRect.top() - rectToAlign.height(), testCollisionRect.bottom());
            collisionFreeZones = subtract(collisionFreeZones, collisionZone);
        }

        double currentBestDistance = std::numeric_limits<double>::max();
        double targetY = 0;

        if (collisionFreeZones.empty())
        {
            continue;
        }

        for (auto& zone : collisionFreeZones)
        {
            if (rectToAlign.y() >= zone.start && rectToAlign.y() <= zone.end)
            {
                targetY = rectToAlign.y();
                break;
            }

            double distance = std::abs(rectToAlign.y() - zone.start);
            if (distance < currentBestDistance)
            {
                currentBestDistance = distance;
                targetY = zone.start;
            }

            distance = std::abs(rectToAlign.y() - zone.end);
            if (distance < currentBestDistance)
            {
                currentBestDistance = distance;
                targetY = zone.end;
            }
        }

        horizontalDisplacement.setY(targetY - rectToAlign.y());

        double displacementSq = horizontalDisplacement.x() * horizontalDisplacement.x() + horizontalDisplacement.y() * horizontalDisplacement.y();
        if (displacementSq < currentBestDisplacement)
        {
            currentBestDisplacement = displacementSq;
            bestDisplacement = horizontalDisplacement;
        }
    }
    return bestDisplacement;
}

QPointF findBestLeftDisplacement(QGraphicsRectItem* rectItemToAlign, const QList<QGraphicsRectItem*>& rectItems)
{
    QRectF rectToAlign = rectItemToAlign->rect().translated(rectItemToAlign->pos());
    QList<QRectF> existingRects;
    for (auto& rectItem : rectItems)
    {
        existingRects.append(rectItem->rect().translated(rectItem->pos()));
    }

    return findBestLeftDisplacement(rectToAlign, existingRects);
}

QPointF findBestRightDisplacement(QGraphicsRectItem* rectItemToAlign, const QList<QGraphicsRectItem*>& rectItems)
{
    QRectF rectToAlign = quarterTurnClockwise(quarterTurnClockwise(rectItemToAlign->rect().translated(rectItemToAlign->pos())));
    QList<QRectF> existingRects;
    for (auto& rectItem : rectItems)
    {
        existingRects.append(quarterTurnClockwise(quarterTurnClockwise(rectItem->rect().translated(rectItem->pos()))));
    }

    return quarterTurnCounterClockwise(quarterTurnCounterClockwise(findBestLeftDisplacement(rectToAlign, existingRects)));
}

QPointF findBestTopDisplacement(QGraphicsRectItem* rectItemToAlign, const QList<QGraphicsRectItem*>& rectItems)
{
    QRectF rectToAlign = quarterTurnClockwise(quarterTurnClockwise(quarterTurnClockwise(rectItemToAlign->rect().translated(rectItemToAlign->pos()))));
    QList<QRectF> existingRects;
    for (auto& rectItem : rectItems)
    {
        existingRects.append(quarterTurnClockwise(quarterTurnClockwise(quarterTurnClockwise(rectItem->rect().translated(rectItem->pos())))));
    }

    return quarterTurnCounterClockwise(quarterTurnCounterClockwise(quarterTurnCounterClockwise(findBestLeftDisplacement(rectToAlign, existingRects))));
}

QPointF findBestBottomDisplacement(QGraphicsRectItem* rectItemToAlign, const QList<QGraphicsRectItem*>& rectItems)
{
    QRectF rectToAlign = quarterTurnClockwise(rectItemToAlign->rect().translated(rectItemToAlign->pos()));
    QList<QRectF> existingRects;
    for (auto& rectItem : rectItems)
    {
        existingRects.append(quarterTurnClockwise(rectItem->rect().translated(rectItem->pos())));
    }

    return quarterTurnCounterClockwise(findBestLeftDisplacement(rectToAlign, existingRects));
}

inline double normSq(const QPointF& point)
{
    return point.x() * point.x() + point.y() * point.y();
}

QPointF minimumDisplacementToAlignRects(QGraphicsRectItem* rectItemToAlign, const QList<QGraphicsRectItem*>& rectItems)
{
    QList<QPointF> candidates;
    candidates << findBestTopDisplacement(rectItemToAlign, rectItems);
    candidates << findBestLeftDisplacement(rectItemToAlign, rectItems);
    candidates << findBestRightDisplacement(rectItemToAlign, rectItems);
    candidates << findBestBottomDisplacement(rectItemToAlign, rectItems);

    int index = 0;
    for (int i = 1; i < candidates.size(); i++)
    {
        if (normSq(candidates[i]) < normSq(candidates[index]))
        {
            index = i;
        }
    }
    return candidates[index];
}

bool rectsConnected(QGraphicsRectItem* rectItem1, QGraphicsRectItem* rectItem2)
{
    QRectF rect1 = rectItem1->rect().translated(rectItem1->pos());
    QRectF rect2 = rectItem2->rect().translated(rectItem2->pos());

    if ((rect1.left() == rect2.right()) && intersects(Interval(rect1.top(), rect1.bottom()), Interval(rect2.top(), rect2.bottom()))) return true;
    if ((rect1.right() == rect2.left()) && intersects(Interval(rect1.top(), rect1.bottom()), Interval(rect2.top(), rect2.bottom()))) return true;
    if ((rect1.top() == rect2.bottom()) && intersects(Interval(rect1.left(), rect1.right()), Interval(rect2.left(), rect2.right()))) return true;
    if ((rect1.bottom() == rect2.top()) && intersects(Interval(rect1.left(), rect1.right()), Interval(rect2.left(), rect2.right()))) return true;

    return false;
}

bool rectsConnected(const QList<QGraphicsRectItem*>& rects)
{
    if (rects.size() <= 1)
    {
        return true;
    }
    QSet<QGraphicsRectItem*> candidates;
    QList<QGraphicsRectItem*> connectedComponent;

    for (int i = 1; i < rects.size(); i++)
    {
        candidates.insert(rects[i]);
    }
    connectedComponent.append(rects[0]);

    while (!candidates.isEmpty())
    {
        int originalCandidatesSize = candidates.size();
        for (auto candidateRect : candidates)
        {
            for (auto connectedRect : connectedComponent)
            {
                if (rectsConnected(connectedRect, candidateRect))
                {
                    candidates.remove(candidateRect);
                    connectedComponent.append(candidateRect);
                    break;
                }
            }
        }
        if (candidates.size() == originalCandidatesSize)
        {
            return false;
        }
    }

    return true;
}

//rectsToConnect can contain mainRect
QList<QGraphicsRectItem*> connectedComponent(QGraphicsRectItem* mainRect, const QList<QGraphicsRectItem*>& rectsToConnect)
{
    QList<QGraphicsRectItem*> result;
    result.append(mainRect);

    int addedRects;
    do
    {
        addedRects = 0;
        for (auto& rectToConnect : rectsToConnect)
        {
            if (result.contains(rectToConnect)) continue;

            bool connected = false;
            for (auto& connectedRect : result)
            {
                if (rectsConnected(connectedRect, rectToConnect))
                {
                    connected = true;
                    break;
                }
            }
            if (connected)
            {
                result.append(rectToConnect);
                addedRects++;
            }
        }
    } while (addedRects != 0);

    return result;
}

//returns modified items
QList<QGraphicsRectItem*> makeRectsConnected(QGraphicsRectItem* mainRect, const QList<QGraphicsRectItem*>& allRects)
{
    QList<QGraphicsRectItem*> modifiedItems;

    auto connectedRects = connectedComponent(mainRect, allRects);

    while (connectedRects.size() != (allRects.size()))
    {
        QPointF minimalDisplacement(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
        QGraphicsRectItem* minimalDisplacementItem = nullptr;

        for (auto& candidateRectItem : allRects)
        {
            if (connectedRects.contains(candidateRectItem)) continue;

            auto candidateRectItemDisplacement = minimumDisplacementToAlignRects(candidateRectItem, connectedRects);
            if (normSq(candidateRectItemDisplacement) < normSq(minimalDisplacement))
            {
                minimalDisplacement = candidateRectItemDisplacement;
                minimalDisplacementItem = candidateRectItem;
            }
        }

        if (minimalDisplacementItem)
        {
            minimalDisplacementItem->setPos(minimalDisplacementItem->pos() + minimalDisplacement);
            connectedRects.append(minimalDisplacementItem);
            modifiedItems.append(minimalDisplacementItem);
        }
        else
        {
            qCritical() << "MonitorRectItem::mouseReleaseEvent critical error";
        }
    }

    return modifiedItems;
}

}

class MonitorRectItem : public QGraphicsRectItem
{
public:
    MonitorRectItem(const Screen* screen);

public:
    void setSelected(bool flag);
    bool selected();
    const Screen* screen();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    bool _hover = false;
    bool _selected = false;
    QString _title = QString();
    const Screen* _screen;
    QPointF _dragStart;
};

class MonitorLayoutEditorScene : public QGraphicsScene
{
public:
    MonitorLayoutEditorScene();

public:
    void select(MonitorRectItem* selectedItem);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};


MonitorLayoutEditor::MonitorLayoutEditor(QWidget* parent) : QGraphicsView(parent)
{
    auto scene = new MonitorLayoutEditorScene();
    setScene(scene);
}

void MonitorLayoutEditor::addScreen(const Screen* screen)
{
    auto currentItem = new MonitorRectItem(screen);
    scene()->addItem(currentItem);

    if(screen->isPrimary())
    {
        currentItem->setSelected(true);
    }
}

void MonitorLayoutEditor::clearScreens()
{
    scene()->clear();
}

void MonitorLayoutEditor::alignScreens()
{
    QList<QGraphicsRectItem*> rectItems;
    QList<QGraphicsRectItem*> alignedItems;
    for (auto item : scene()->items())
    {
        if (auto rectItem = dynamic_cast<MonitorRectItem*>(item))
        {
            rectItems << rectItem;
            if (rectItem->screen()->isPrimary())
            {
                alignedItems << rectItem;
            }
        }
    }

    if (rectItems.isEmpty()) return;
    if (alignedItems.isEmpty())
    {
        alignedItems.append(rectItems[0]);
    }

    for (auto& rectItem : rectItems)
    {
        if (!alignedItems.contains(rectItem))
        {
            auto displacement = minimumDisplacementToAlignRects(rectItem, alignedItems);
            if (displacement != QPointF(0.0, 0.0))
            {
                rectItem->setPos(rectItem->pos() + displacement);
                emit screenGeometryChanged(static_cast<MonitorRectItem*>(rectItem)->screen(), rectItem->rect().translated(rectItem->pos()));
            }
            alignedItems.append(rectItem);
        }
    }

}

const Screen* MonitorLayoutEditor::selectedScreen()
{
    for (auto item : scene()->items())
    {
        if (auto rectItem = dynamic_cast<MonitorRectItem*>(item))
        {
            if (rectItem->selected())
            {
                return rectItem->screen();
            }
        }
    }
    return nullptr;
}

void MonitorLayoutEditor::tryModifyScreenGeometry(const Screen* screen, QRectF& geometry)
{
    MonitorRectItem* targetItem = nullptr;
    QList<QGraphicsRectItem*> otherItems;
    QList<QGraphicsRectItem*> allItems;
    for (auto item : scene()->items())
    {
        if (auto rectItem = dynamic_cast<MonitorRectItem*>(item))
        {
            allItems.append(rectItem);
            if (rectItem->screen() == screen)
            {
                targetItem = rectItem;
            }
            else
            {
                otherItems << rectItem;
            }
        }
    }

    if (!targetItem) return;

    targetItem->setRect(QRectF(0,0, geometry.width(), geometry.height()));
    targetItem->setPos(QPointF(geometry.x(), geometry.y()));
    targetItem->setPos(targetItem->pos() + minimumDisplacementToAlignRects(targetItem, otherItems));
    
    if (!rectsConnected(allItems))
    {
        auto modifiedItems = makeRectsConnected(targetItem, allItems);
        for (auto& item : modifiedItems)
        {
            emit screenGeometryChanged(static_cast<MonitorRectItem*>(item)->screen(), item->rect().translated(item->pos()));
        }
    }
    
    geometry = targetItem->rect().translated(targetItem->pos());

    emit screenGeometryChanged(screen, geometry);
}

void MonitorLayoutEditor::zoom(double factor)
{
    setTransform(transform().scale(factor, factor));
}

void MonitorLayoutEditor::zoomToFit()
{
    QRectF rectUnion(0, 0, 0, 0);
    for (auto& item : scene()->items())
    {
        if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(item))
        {
            if (rectUnion.isNull())
            {
                rectUnion = rectItem->rect().translated(rectItem->pos());
            }
            else
            {
                rectUnion = rectUnion.united(rectItem->rect().translated(rectItem->pos()));
            }
        }
    }

    scene()->setSceneRect(rectUnion);
    fitInView(rectUnion.adjusted(-50,-50,50,50), Qt::KeepAspectRatio);
}

void MonitorLayoutEditor::wheelEvent(QWheelEvent* event)
{
    if(event->angleDelta().y() > 0)
    {
        zoom(1.1);
    }
    else
    {
        zoom(0.9);
    }
    QGraphicsView::wheelEvent(event);
}

void MonitorLayoutEditor::mouseDoubleClickEvent(QMouseEvent* event)
{
    zoomToFit();
    QGraphicsView::mouseDoubleClickEvent(event);
}



MonitorRectItem::MonitorRectItem(const Screen* screen) : QGraphicsRectItem()
{
    _screen = screen;
    auto displayPhysicalRect = screen->physicalRect();
    setRect(0, 0, displayPhysicalRect.width(), displayPhysicalRect.height());
    setPos(displayPhysicalRect.x(), displayPhysicalRect.y());
    _title = screen->name();
    for (auto& monitorHardwareNames : screen->hardwareNames())
    {
        _title = _title + QString(_title.isEmpty() ? "" : "\n") + monitorHardwareNames;
    }
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptHoverEvents(true);
}

void MonitorRectItem::setSelected(bool flag)
{
    _selected = flag;
    setZValue(_selected ? 1 : 0);
    update();
}

bool MonitorRectItem::selected()
{
    return _selected;
}

const Screen* MonitorRectItem::screen()
{
    return _screen;
}

void MonitorRectItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    static_cast<MonitorLayoutEditorScene*>(scene())->select(this);
    _dragStart = pos();
    QGraphicsRectItem::mousePressEvent(event);
}

void MonitorRectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (pos() == _dragStart)
    {
        return;
    }
    QList<QGraphicsRectItem*> allRectItems;
    QList<QGraphicsRectItem*> otherRectItems;

    for (auto& item : scene()->items())
    {
        if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(item))
        {
            allRectItems << rectItem;
            if (rectItem != this)
            {
                otherRectItems << rectItem;
            }
        }
    }

    QList<QGraphicsRectItem*> allModifiedItems;

    setPos(pos() + minimumDisplacementToAlignRects(this, otherRectItems));
    allModifiedItems.append(this);

    if (!rectsConnected(allRectItems))
    {
        auto modifiedItems = makeRectsConnected(this, allRectItems);
        for (auto& item : modifiedItems)
        {
            if (!allModifiedItems.contains(item))
            {
                allModifiedItems.append(item);
            }
        }
    }

    QPointF origin(0.0, 0.0);
    for (auto& item : scene()->items())
    {
        if (auto rectItem = dynamic_cast<MonitorRectItem*>(item))
        {
            if (rectItem->screen()->isPrimary())
            {
                origin = rectItem->pos();
            }
        }
    }

    if (origin != QPointF(0.0, 0.0))
    {
        for (auto& item : scene()->items())
        {
            if (auto rectItem = dynamic_cast<MonitorRectItem*>(item))
            {
                rectItem->setPos(rectItem->pos() - origin);
                if (!allModifiedItems.contains(rectItem))
                {
                    allModifiedItems.append(rectItem);
                }
            }
        }
    }
    
    for (auto& item : allModifiedItems)
    {
        for (auto view : scene()->views())
        {
            if (auto layoutEditor = dynamic_cast<MonitorLayoutEditor*>(view))
            {
                emit layoutEditor->screenGeometryChanged(static_cast<MonitorRectItem*>(item)->screen(), item->rect().translated(item->pos()));
            }
        }
    }
    QGraphicsRectItem::mouseReleaseEvent(event);
}

void MonitorRectItem::keyPressEvent(QKeyEvent* event)
{
    if (!isSelected())
    {
        return;
    }
    if (event->key() == Qt::Key_Up)
    {
        setPos(pos() + QPointF(0, -1));
    }
    if (event->key() == Qt::Key_Down)
    {
        setPos(pos() + QPointF(0, 1));
    }
    if (event->key() == Qt::Key_Left)
    {
        setPos(pos() + QPointF(-1, 0));
    }
    if (event->key() == Qt::Key_Right)
    {
        setPos(pos() + QPointF(1, 0));
    }
    QGraphicsRectItem::keyPressEvent(event);
}

void MonitorRectItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    _hover = true;
    QGraphicsRectItem::hoverEnterEvent(event);
}

void MonitorRectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    _hover = false;
    QGraphicsRectItem::hoverLeaveEvent(event);
}

void MonitorRectItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (_selected)
    {
        if (_hover)
        {
            painter->setBrush(QColor("#49C9D1"));
        }
        else
        {
            painter->setBrush(QColor("#00B7C3"));
        }
    }
    else
    {
        if (_hover)
        {
            painter->setBrush(QColor("#C2C2C2"));
        }
        else
        {
            painter->setBrush(QColor("#DADADA"));
        }
    }
    painter->setPen(QPen(Qt::transparent, 3));
    int padding = 2;
    painter->drawRoundedRect(rect().adjusted(padding,padding,-padding,-padding), 12, 12);

    if (_selected)
    {
        painter->setPen(Qt::white);
    }
    else
    {
        painter->setPen(Qt::black);
    }

    QFont font("Segoe UI", 22);
    QRect titleRect   = QFontMetrics(font).boundingRect(_title);
    qreal fontFactorw = rect().width() / (qreal)titleRect.width();
    qreal fontFactorh = rect().height() / (qreal)titleRect.height();
    qreal fontFactor = qMin(fontFactorw, fontFactorh);
    fontFactor = qMin(fontFactor * 0.75, 1.0);
    font.setPointSizeF(std::max(font.pointSizeF() * fontFactor, 1.0));
    painter->setFont(font);

    painter->drawText(rect(), Qt::AlignCenter, _title);
}

MonitorLayoutEditorScene::MonitorLayoutEditorScene() : QGraphicsScene()
{
}

void MonitorLayoutEditorScene::select(MonitorRectItem* selectedItem)
{
    for (auto& item : items())
    {
        if (dynamic_cast<MonitorRectItem*>(item))
        {
            if (item != selectedItem)
            {
                dynamic_cast<MonitorRectItem*>(item)->setSelected(false);
            }
            else
            {
                dynamic_cast<MonitorRectItem*>(item)->setSelected(true);
            }
        }
    }
    for (auto& view : views())
    {
        if (auto editor = dynamic_cast<MonitorLayoutEditor*>(view))
        {
            emit editor->screenSelectionChanged(selectedItem->screen());
        }
    }
    //_monitorWidget->setDisplaySettings(selectedItem->screen());
}

void MonitorLayoutEditorScene::keyPressEvent(QKeyEvent* event)
{
    for (auto& item : items())
    {
        sendEvent(item, event);
    }
    QGraphicsScene::keyPressEvent(event);
}

}