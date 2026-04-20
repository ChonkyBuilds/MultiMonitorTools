#include "MMTSettings.hpp"

#include <QSettings>
#include <QMetaEnum>
#include <QIcon>
#include <QPainter>
#include <QDir>

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
    bool customTrayIconEnabled();
    void setCustomTrayIconEnabled(bool);
    QIcon customTrayIconForDesktop(const QString& desktopName);
    void setCustomTrayIconPathForDesktop(const QString& desktopName, const QString& iconPath);
    void setCustomTrayIconTextForDesktop(const QString& desktopName, const QString& iconText);

private:
    std::unique_ptr<QSettings> _settings;
    QMap<QString, QIcon> _iconCache;
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

bool SettingsImpl::customTrayIconEnabled()
{
    return _settings->value("CustomTrayIconEnabled", false).toBool();
}

void SettingsImpl::setCustomTrayIconEnabled(bool enabled)
{
    _settings->setValue("CustomTrayIconEnabled", enabled);
}

QIcon SettingsImpl::customTrayIconForDesktop(const QString& desktopName)
{
    if (desktopName.isEmpty()) return QIcon();

    if (_iconCache.contains(desktopName))
    {
        return _iconCache[desktopName];;
    }

    QIcon icon;

    _settings->beginGroup("DesktopTrayIcons");
    _settings->beginGroup(desktopName);
    auto iconPath = _settings->value("IconPath").toString();
    if (!iconPath.isEmpty())
    {
        icon = QIcon(iconPath);
    }
    else
    {
        QIcon baseIcon(":/resources/AppIcon.ico");
        QPixmap pixmap = baseIcon.pixmap(QSize(256, 256));
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        QFont font = painter.font();
        font.setPixelSize(175);
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(Qt::white);
        QRect rect = pixmap.rect().adjusted(0,0,0,-60);
        painter.drawText(rect, Qt::AlignCenter, _settings->value("IconText", desktopName[0]).toString());
        painter.end();

        icon = QIcon(pixmap);
    }

    _settings->endGroup();
    _settings->endGroup();
    
    _iconCache[desktopName] = icon;
    return icon;
}

void SettingsImpl::setCustomTrayIconPathForDesktop(const QString& desktopName, const QString& iconPath)
{
    _iconCache.remove(desktopName);

    QString path = "customIcons";
    QDir dir;
    if (!dir.exists(path)) 
    {
        dir.mkpath(path);
    }

    QString internalPath = path + "/" + desktopName + "." + QFileInfo(iconPath).suffix();
    if (QFile::exists(internalPath)) 
    {
        QFile::remove(internalPath);
    }
    if (!QFile::copy(iconPath, internalPath))
    {
        qWarning() << "Failed to copy icon from " << iconPath << " to " << internalPath;
        return;
    }

    _settings->beginGroup("DesktopTrayIcons");
    _settings->beginGroup(desktopName);
    _settings->setValue("IconPath", internalPath);
    _settings->setValue("IconText", "");
    _settings->endGroup();
    _settings->endGroup();
}

void SettingsImpl::setCustomTrayIconTextForDesktop(const QString& desktopName, const QString& iconText)
{
    _iconCache.remove(desktopName);

    _settings->beginGroup("DesktopTrayIcons");
    _settings->beginGroup(desktopName);
    _settings->setValue("IconText", iconText);
    _settings->setValue("IconPath", "");
    _settings->endGroup();
    _settings->endGroup();
}

Settings* Settings::instance()
{
    static SettingsImpl inst;
    return &inst;
}

}