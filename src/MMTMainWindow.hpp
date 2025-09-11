#pragma once

#include "MMT.hpp"
#include <QtWidgets/QMainWindow>
#include <QSystemTrayIcon>

class QTextEdit;
class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QTimer;
class QString;
class QCloseEvent;

namespace MMT{

class MonitorConfigWidget;
class ScreenOverlay;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void appendMessage(const QString&, QtMsgType);

private slots:
    void onEventLoopStarted();
    void systemTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onPinWindows();
    void onSwitchVirtualDesktop(Direction direction, bool bringWindowInFocus);
    void toggleOutputConsole();
    void onScreenGeometryChanged();
    void toggleCursorAdjust(bool);
    void toggleDrawGridLines(bool);
    void enableAnimation(bool);
    void onEasingCurveChanged(const QString&);
    void onAnimationDurationChanged(int);
    void onLogLevelChanged(const QString&);
    void onGridLineGapChanged(double);
    void updateGridLines();
    void setupOverlays();
    void onMonitorInfoRefreshed();
    void onAbout();

protected:
    void closeEvent(QCloseEvent* evt) override;

private:
    QSystemTrayIcon* _systemTray = nullptr;
    QTextEdit* _outputConsole = nullptr;
    std::vector<std::unique_ptr<MMT::ScreenOverlay>> _screenOverlays;
    QTimer* _pinWindowsTimer = nullptr;
    QComboBox* _logLevel = nullptr;
    QCheckBox* _drawGridLines = nullptr;
    QDoubleSpinBox* _gridLineGap = nullptr;
    MonitorConfigWidget* _monitorConfigWidget = nullptr;
    QTimer* _cursorAdjustRestartCooldown = nullptr;
};

}