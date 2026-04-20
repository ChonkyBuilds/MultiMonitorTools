#pragma once

#include <QEasingCurve>

class QString;
class QIcon;

namespace MMT
{

class Settings
{
public:
    static Settings* instance();

public:
    virtual bool cursorAdjustEnabled() = 0;
    virtual void setCursorAdjustEnabled(bool) = 0;

    virtual bool animationEnabled() = 0;
    virtual void setAnimationEnabled(bool) = 0;

    virtual QString logLevel() = 0;
    virtual void setLogLevel(const QString&) = 0;

    virtual bool showOutputConsole() = 0;
    virtual void setShowOutputConsole(bool) = 0;

    virtual QEasingCurve::Type easingCurve() = 0;
    virtual QString easingCurveString() = 0;
    virtual void setEasingCurveString(const QString&) = 0;

    virtual int animationDuration() = 0;
    virtual void setAnimationDuration(int) = 0;

    virtual bool customTrayIconEnabled() = 0;
    virtual void setCustomTrayIconEnabled(bool) = 0;

    virtual QIcon customTrayIconForDesktop(const QString& desktopName) = 0;
    virtual void setCustomTrayIconPathForDesktop(const QString& desktopName, const QString& iconPath) = 0;
    virtual void setCustomTrayIconTextForDesktop(const QString& desktopName, const QString& iconText) = 0;

};

}