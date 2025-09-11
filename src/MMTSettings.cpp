#include "MMTSettings.hpp"

#include <QSettings>
#include <QMetaEnum>

namespace MMT
{

class SettingsImpl : public Settings
{
public:
    SettingsImpl();
    ~SettingsImpl();
public:
    bool cursorAdjustEnabled();
    void setCursorAdjustEnabled(bool);
    bool animationEnabled();
    void setAnimationEnabled(bool);
    QString logLevel();
    void setLogLevel(const QString&);
    bool showOutputConsole();
    void setShowOutputConsole(bool);
    QEasingCurve::Type easingCurve();
    QString easingCurveString();
    void setEasingCurveString(const QString&);
    int animationDuration();
    void setAnimationDuration(int);

private:
    std::unique_ptr<QSettings> _settings;
};

SettingsImpl::SettingsImpl()
{
    _settings = std::make_unique<QSettings>("GlobalSettings.ini", QSettings::IniFormat);
}

SettingsImpl::~SettingsImpl()
{
    _settings->sync();
}

bool SettingsImpl::cursorAdjustEnabled()
{
    return _settings->value("CursorAdjustEnabled", true).toBool();
}

void SettingsImpl::setCursorAdjustEnabled(bool enabled)
{
    _settings->setValue("CursorAdjustEnabled", enabled);
}

bool SettingsImpl::animationEnabled()
{
    return _settings->value("AnimationEnabled", true).toBool();
}

void SettingsImpl::setAnimationEnabled(bool enabled)
{
    _settings->setValue("AnimationEnabled", enabled);
}

QString SettingsImpl::logLevel()
{
    return _settings->value("LogLevel", "Warning").toString();
}

void SettingsImpl::setLogLevel(const QString& level)
{
    _settings->setValue("LogLevel", level);
}

bool SettingsImpl::showOutputConsole()
{
    return _settings->value("ShowOutputConsole", false).toBool();
}

void SettingsImpl::setShowOutputConsole(bool enabled)
{
    _settings->setValue("ShowOutputConsole", enabled);
}

QEasingCurve::Type SettingsImpl::easingCurve()
{
    auto easingCurveString = _settings->value("EasingCurve", "InOutQuad").toString();
    return QEasingCurve::Type(QEasingCurve::staticMetaObject.enumerator(QEasingCurve::staticMetaObject.indexOfEnumerator("Type")).keyToValue(easingCurveString.toLocal8Bit()));
}

QString SettingsImpl::easingCurveString()
{
    return _settings->value("EasingCurve", "InOutQuad").toString();;
}

void SettingsImpl::setEasingCurveString(const QString& easingCurveString)
{
    _settings->setValue("EasingCurve", easingCurveString);
}

int SettingsImpl::animationDuration()
{
    return _settings->value("AnimationDuration", 400).toInt();
}

void SettingsImpl::setAnimationDuration(int duration)
{
    _settings->setValue("AnimationDuration", duration);
}

Settings* Settings::instance()
{
    static SettingsImpl inst;
    return &inst;
}

}