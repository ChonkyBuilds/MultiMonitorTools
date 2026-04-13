#pragma once

#include <QObject>
#include <QList>

class QPixmap;
class QRect;
class QRectF;
class QString;

namespace MMT
{

/*
"screen" is an abstract surface to display on
"monitor" is a piece of hardware that physically displays contents of a "screen"
*/
class Screen
{
public:
    virtual QRectF physicalRect() const = 0;
    virtual void setPhysicalRect(const QRectF&) = 0;
    virtual bool virtualDesktopEnabled() const = 0;
    virtual void setVirtualDesktopEnabled(bool) = 0;

    //physical and logical coordinates differ because of dpi scalling
    virtual QRect physicalCoordinateRect() const = 0;
    virtual QRect logicalCoordinateRect() const = 0;
    virtual QString name() const = 0;
    virtual QList<QString> hardwareNames() const = 0;
    virtual QList<QString> hardwareEDIDHeaders() const = 0;
    virtual bool isValid() const = 0;
    virtual bool isPrimary() const = 0;
    virtual QPixmap captureScreenShot() = 0;
    virtual void* getNativeHandle() = 0;

protected:
    Screen() = default;
    ~Screen() = default;
};

class MonitorManager : public QObject
{
    Q_OBJECT
public:
    static MonitorManager* instance();

public:
    virtual QList<Screen*> screens() = 0;    

public slots:
    virtual void refreshMonitorInfo() = 0;

signals:
    void monitorInfoRefreshed();

protected:
    MonitorManager() = default;
    ~MonitorManager() = default;
    MonitorManager(MonitorManager const&) = delete;
    void operator=(MonitorManager const&) = delete;
};

}