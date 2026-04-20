#pragma once

#include <QWidget>

namespace MMT
{
class TrayIconManager : public QWidget
{
    Q_OBJECT
public:
    explicit TrayIconManager(QWidget* parent = nullptr);
    ~TrayIconManager();

private slots:
    void checkDesktopChange();
    void onEventLoopStarted();

signals:
    void updateTrayIcon(const QIcon& icon);

private:
    QString _currentDesktopName = "";
};
}

