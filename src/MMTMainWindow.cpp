#pragma once

#include "MMTMainWindow.hpp"
#include "MMTKeyboardHook.hpp"
#include "MMTVirtualDesktop.hpp"
#include "MMTCursorAdjust.hpp"
#include "MMTMonitorManager.hpp"
#include "MMTSettings.hpp"
#include "MMTMonitorConfigWidget.hpp"
#include "MMTScreenOverlay.hpp"
#include "MMTHotkeys.hpp"
#include "MMTHotkeyEditor.hpp"

#include <QHboxLayout>
#include <QPushButton>
#include <QLabel>
#include <qDebug>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QThread>
#include <QScreen>
#include <QTimer>
#include <QStatusBar>
#include <QGroupBox>
#include <QtConcurrent>
#include <QMenuBar>
#include <QSplitter>
#include <QSpinBox>
#include <QOverload>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wtsapi32.h>
#pragma comment(lib, "Wtsapi32.lib")

namespace MMT {

class Separator : public QFrame
{
public:
    Separator(QWidget* parent = nullptr) : QFrame(parent)
    {
        setFrameShape(QFrame::VLine);
        setFrameShadow(QFrame::Raised);
    }
};

class WindowsSessionWatcher : public QAbstractNativeEventFilter
{
public:
    std::function<void()> onSessionChanged = nullptr;
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *) override
    {
        if (eventType == "windows_generic_MSG")
        {
            MSG* msg = static_cast<MSG*>(message);

            if (msg->message == WM_WTSSESSION_CHANGE)
            {
                if (onSessionChanged) onSessionChanged();
            }
        }
        return false;
    }
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    qRegisterMetaType<Direction>("Direction");
    qRegisterMetaType<Screen>("Screen");
    qRegisterMetaType<Hotkey>("Hotkey");

    qApp->setFont(QFont("Segoe UI"));

    QIcon icon(":/resources/AppIcon.ico");

    _systemTray = new QSystemTrayIcon(this);
    _systemTray->setIcon(icon);
    QObject::connect(_systemTray, &QSystemTrayIcon::activated, this, &MainWindow::systemTrayActivated);
    auto systemTrayMenu = new QMenu(this);
    auto quitAction = new QAction(MainWindow::tr("Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    systemTrayMenu->addAction(quitAction);
    _systemTray->setContextMenu(systemTrayMenu);
    _systemTray->show();

    auto centralWidget = new QWidget;
    setCentralWidget(centralWidget);
    auto centralWidgetLayout = new QVBoxLayout(centralWidget);
    
    auto toggleOutputConsole = new QPushButton("Toggle output console");
    QObject::connect(toggleOutputConsole, &QPushButton::clicked, this, &MainWindow::toggleOutputConsole);

    auto toggleCursorAdjust = new QCheckBox("Adjust cursor position when crossing monitors");
    toggleCursorAdjust->setChecked(Settings::instance()->cursorAdjustEnabled());
    QObject::connect(toggleCursorAdjust, &QCheckBox::toggled, this, &MainWindow::toggleCursorAdjust);

    _drawGridLines = new QCheckBox("Draw grid lines");
    _drawGridLines->setChecked(false);
    QObject::connect(_drawGridLines, &QCheckBox::toggled, this, &MainWindow::toggleDrawGridLines);

    _gridLineGap = new QDoubleSpinBox;
    _gridLineGap->setMinimum(1);
    _gridLineGap->setMaximum(99);
    _gridLineGap->setValue(15);
    _gridLineGap->setEnabled(false);
    QObject::connect(_gridLineGap, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onGridLineGapChanged);

    auto enableAnimation = new QCheckBox("Enable animation");
    enableAnimation->setChecked(Settings::instance()->animationEnabled());
    QObject::connect(enableAnimation, &QCheckBox::toggled, this, &MainWindow::enableAnimation);

    auto easingCurveComboBox = new QComboBox();
    QMetaEnum easingCurveTypeEnum = QEasingCurve::staticMetaObject.enumerator(QEasingCurve::staticMetaObject.indexOfEnumerator("Type"));
    for (int i = 0; i < QEasingCurve::BezierSpline; i++) 
    {
        easingCurveComboBox->addItem(easingCurveTypeEnum.valueToKey(i));
    }
    easingCurveComboBox->setCurrentText(Settings::instance()->easingCurveString());
    QObject::connect(easingCurveComboBox, &QComboBox::currentTextChanged, this, &MainWindow::onEasingCurveChanged);

    auto animationDuration = new QSpinBox;
    animationDuration->setMinimum(100);
    animationDuration->setMaximum(9999);
    animationDuration->setSingleStep(10);
    animationDuration->setValue(Settings::instance()->animationDuration());
    QObject::connect(animationDuration, &QSpinBox::valueChanged, this, &MainWindow::onAnimationDurationChanged);

    _logLevel = new QComboBox;
    _logLevel->addItem("Debug");
    _logLevel->addItem("Info");
    _logLevel->addItem("Warning");
    _logLevel->addItem("Critical");
    _logLevel->addItem("Fatal");
    _logLevel->setCurrentText(Settings::instance()->logLevel());

    QObject::connect(_logLevel, &QComboBox::currentTextChanged, this, &MainWindow::onLogLevelChanged);

    auto aboutButton = new QPushButton("About");
    QObject::connect(aboutButton, &QPushButton::clicked, this, &MainWindow::onAbout);

    auto hotkeyEditorButton = new QPushButton("Hotkeys");
    QObject::connect(hotkeyEditorButton, &QPushButton::clicked, this, &MainWindow::onEditHotkeys);

    auto logLevelWidget = new QWidget;
    auto logLevelWidgetLayout = new QHBoxLayout(logLevelWidget);
    logLevelWidgetLayout->setContentsMargins(0, 0, 0, 0);
    logLevelWidgetLayout->addWidget(new QLabel("Log level:"));
    logLevelWidgetLayout->addWidget(_logLevel);
    statusBar()->addPermanentWidget(hotkeyEditorButton);
    statusBar()->addPermanentWidget(aboutButton);
    statusBar()->addPermanentWidget(logLevelWidget);
    statusBar()->addPermanentWidget(toggleOutputConsole);

    auto monitorGroupBox = new QGroupBox(MainWindow::tr("Monitor Settings"));
    _monitorConfigWidget = new MonitorConfigWidget();
    QObject::connect(_monitorConfigWidget, &MonitorConfigWidget::screenGeometryChanged, this, &MainWindow::onScreenGeometryChanged);
    QObject::connect(_monitorConfigWidget, &MonitorConfigWidget::screenGeometryChanged, this, &MainWindow::updateGridLines);
    auto monitorGroupBoxLayout = new QVBoxLayout(monitorGroupBox);
    monitorGroupBoxLayout->setContentsMargins(0, 0, 0, 0);
    monitorGroupBoxLayout->addWidget(_monitorConfigWidget);
    
    auto cursorAdjustSettings = new QGroupBox("Cursor Settings");
    auto cursorAdjustSettingsLayout = new QVBoxLayout(cursorAdjustSettings);
    cursorAdjustSettingsLayout->addWidget(toggleCursorAdjust);
    auto drawGridLinesLayout = new QHBoxLayout();
    drawGridLinesLayout->setContentsMargins(0, 0, 0, 0);
    drawGridLinesLayout->addWidget(_drawGridLines);
    drawGridLinesLayout->addWidget(new Separator());
    drawGridLinesLayout->addWidget(new QLabel("Line gap(mm):"));
    drawGridLinesLayout->addWidget(_gridLineGap);
    drawGridLinesLayout->addStretch();
    cursorAdjustSettingsLayout->addLayout(drawGridLinesLayout);

    auto virtualDesktopSettings = new QGroupBox("Virtual Desktop Settings");
    auto virtualDesktopSettingsLayout = new QVBoxLayout(virtualDesktopSettings);
    virtualDesktopSettingsLayout->addWidget(enableAnimation);
    auto easingCurveLayout = new QHBoxLayout;
    easingCurveLayout->addWidget(new QLabel("Animation easing curve:"));
    easingCurveLayout->addWidget(easingCurveComboBox);
    easingCurveLayout->addStretch();
    virtualDesktopSettingsLayout->addLayout(easingCurveLayout);
    auto animationDurationLayout = new QHBoxLayout;
    animationDurationLayout->addWidget(new QLabel("Animation duration(ms):"));
    animationDurationLayout->addWidget(animationDuration);
    animationDurationLayout->addStretch();
    virtualDesktopSettingsLayout->addLayout(animationDurationLayout);

    auto mainWidget = new QWidget;
    auto mainWidgetLayout = new QVBoxLayout(mainWidget);
    mainWidgetLayout->setContentsMargins(0, 0, 0, 0);
    mainWidgetLayout->addWidget(monitorGroupBox);
    mainWidgetLayout->addWidget(cursorAdjustSettings);
    mainWidgetLayout->addWidget(virtualDesktopSettings);

    auto splitter = new QSplitter;
    centralWidgetLayout->addWidget(splitter);
    splitter->addWidget(mainWidget);
    splitter->setCollapsible(0, false);

    _outputConsole = new QTextEdit;
    _outputConsole->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    _outputConsole->setVisible(Settings::instance()->showOutputConsole());
    splitter->addWidget(_outputConsole);
    splitter->setCollapsible(1, false);

    setupOverlays();
    QObject::connect(MonitorManager::instance(), &MonitorManager::monitorInfoRefreshed, this, &MainWindow::onMonitorInfoRefreshed);

    HotkeyManager::instance()->loadHotkeys();
    QObject::connect(KeyboardHook::instance(), &KeyboardHook::hotkeyTriggered, this, &MainWindow::onHotkeyTriggered);

    _pinWindowsTimer = new QTimer(this);
    _pinWindowsTimer->setSingleShot(false);
    _pinWindowsTimer->setInterval(100);
    QObject::connect(_pinWindowsTimer, &QTimer::timeout, this, &MainWindow::onPinWindows);
    _pinWindowsTimer->start();

    _cursorAdjustRestartTimer = new QTimer(this);
    _cursorAdjustRestartTimer->setSingleShot(true);
    _cursorAdjustRestartTimer->setInterval(1000);
    QObject::connect(_cursorAdjustRestartTimer, &QTimer::timeout, this, &MainWindow::restartCursorAdjust);

    WindowsSessionWatcher* watcher = new WindowsSessionWatcher;
    watcher->onSessionChanged = [this]()
        {
            _cursorAdjustRestartTimer->start(1000);
        };
    
    qApp->installNativeEventFilter(watcher);
    WTSRegisterSessionNotification(HWND(winId()), NOTIFY_FOR_THIS_SESSION);

    QMetaObject::invokeMethod(this, "onEventLoopStarted");
}

void MainWindow::appendMessage(const QString& message, QtMsgType type)
{
    if (_outputConsole)
    {
        //const QStringList msgTypeStringList = {"Debug", "Info", "Warning", "Critical", "Fatal"};
        const QList<QtMsgType> msgTypeList = { QtDebugMsg, QtInfoMsg, QtWarningMsg, QtCriticalMsg, QtFatalMsg};

        if (msgTypeList.indexOf(type) >= 2)
        {
            _outputConsole->setTextColor(Qt::red);
        }
        else
        {
            _outputConsole->setTextColor(_outputConsole->palette().color(QPalette::Text));
        }
        _outputConsole->append(message);
    }
}

MainWindow::~MainWindow()
{
    _pinWindowsTimer->stop();
    CursorAdjust::instance()->exit();
    KeyboardHook::instance()->exit();
}

void MainWindow::onEventLoopStarted()
{
    KeyboardHook::instance()->initiate();
    if (Settings::instance()->cursorAdjustEnabled())
    {
        CursorAdjust::instance()->initiate();
    }
}

void MainWindow::systemTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::DoubleClick:
        break;
    case QSystemTrayIcon::Trigger:
        if (isMaximized() && isMinimized())
        {
            showMaximized();
            break;
        }
        showNormal();
        break;
    default:
        break;
    }
}

void MainWindow::onPinWindows()
{
    LASTINPUTINFO lastInput;
    lastInput.cbSize = sizeof(LASTINPUTINFO);
    lastInput.dwTime = 0;
    if (GetLastInputInfo(&lastInput) != 0)
    {
        if (GetTickCount64() - lastInput.dwTime > 5000)
        {
            return;
        }
    }
    else
    {
        qWarning() << "GetLastInputInfo failed!";
    }

    QList<Screen*> pinScreens;
    auto screens = MonitorManager::instance()->screens();
    for (auto& screen : screens)
    {
        if (!screen->virtualDesktopEnabled())
        {
            pinScreens.append(screen);
        }
    }
    VirtualDesktop::instance()->pinWindowsOnScreen(pinScreens);
}

void MainWindow::onSwitchVirtualDesktop(Direction direction, bool bringWindowInFocus)
{
    if (VirtualDesktop::instance()->adjacentDesktopAvailable(direction))
    {
        QList<QFuture<void>> setupTasks;
        auto setupDone = QSharedPointer<QMap<ScreenOverlay*, bool>>::create();
        for (auto& overlay : _screenOverlays)
        {
            if (overlay->screen()->virtualDesktopEnabled())
            {
                auto overlayPtr = overlay.get();
                setupTasks << QtConcurrent::run(&ScreenOverlay::setupDesktopSwitchAnimation, overlayPtr);
                setupDone->insert(overlayPtr, false);

                QMetaObject::Connection* const connection = new QMetaObject::Connection;
                *connection = connect(overlayPtr, &ScreenOverlay::setupDone,
                    [direction, bringWindowInFocus, setupDone, overlayPtr, connection, this]()
                    {
                        (*setupDone)[overlayPtr] = true;
                        bool allSetupDone = true;
                        for (auto& done : *setupDone)
                        {
                            if (!done) allSetupDone = false;
                        }
                        if (allSetupDone)
                        {
                            VirtualDesktop::instance()->moveToAdjacentDesktop(direction, bringWindowInFocus);
                        }
                        QObject::disconnect(*connection);
                        delete connection;
                    });
            }
        }

        for (auto& setupTask : setupTasks)
        {
            setupTask.waitForFinished();
        }

        for (auto& overlay : _screenOverlays)
        {
            if (overlay->screen()->virtualDesktopEnabled())
            {
                overlay->animateDesktopSwitch(direction);
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* evt)
{
    if (!evt->spontaneous() || !isVisible())
        return;
    QMessageBox::information(this, MainWindow::tr("Systray"), MainWindow::tr("The program will keep running in the background. To terminate the program, please choose <b>Quit</b> in the context menu of the system tray."));
    hide();
    evt->ignore();
}

void MainWindow::toggleOutputConsole()
{
    if (_outputConsole)
    {
        if (_outputConsole->isVisible())
        {
            _outputConsole->hide();
            Settings::instance()->setShowOutputConsole(false);
        }
        else
        {
            _outputConsole->show();
            Settings::instance()->setShowOutputConsole(true);
        }
    }
}

void MainWindow::onScreenGeometryChanged()
{
    if (Settings::instance()->cursorAdjustEnabled())
    {
        _cursorAdjustRestartTimer->start(1000);
    }
}

void MainWindow::toggleCursorAdjust(bool flag)
{
    if (flag)
    {
        CursorAdjust::instance()->initiate();
    }
    else
    {
        CursorAdjust::instance()->exit();
    }
    Settings::instance()->setCursorAdjustEnabled(flag);
}

void MainWindow::toggleDrawGridLines(bool enabled)
{
    updateGridLines();
}

void MainWindow::enableAnimation(bool flag)
{
    Settings::instance()->setAnimationEnabled(flag);
}

void MainWindow::onEasingCurveChanged(const QString& easingCurveTypeString)
{
    Settings::instance()->setEasingCurveString(easingCurveTypeString);
}

void MainWindow::onAnimationDurationChanged(int duration)
{
    Settings::instance()->setAnimationDuration(duration);
}

void MainWindow::onLogLevelChanged(const QString& value)
{
    Settings::instance()->setLogLevel(value);
}

void MainWindow::onGridLineGapChanged(double)
{
    updateGridLines();
}

void MainWindow::updateGridLines()
{
    for (auto& overlay : _screenOverlays)
    {
        if (_drawGridLines->isChecked())
        {
            overlay->drawGridLines(_gridLineGap->value(), {Qt::red, Qt::blue, Qt::green, Qt::magenta, Qt::cyan});
            _gridLineGap->setEnabled(true);
        }
        else
        {
            overlay->drawLines(QList<QPointF>(), QList<QPointF>(), QList<QColor>());
            _gridLineGap->setEnabled(false);
        }
    }
}

void MainWindow::setupOverlays()
{
    _screenOverlays.clear();

    auto screens = MonitorManager::instance()->screens();
    for (auto& screen : screens)
    {
        _screenOverlays.push_back(std::make_unique<MMT::ScreenOverlay>(screen));
    }
    for (auto& overlay : _screenOverlays)
    {
        overlay->show();
    }
}

void MainWindow::onMonitorInfoRefreshed()
{
    setupOverlays();
    _monitorConfigWidget->setupScreens();
}

void MainWindow::onAbout()
{
    QDialog dlg;
    dlg.setWindowTitle("About");

    auto layout = new QVBoxLayout(&dlg);
    auto label = new QLabel(this);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setOpenExternalLinks(true);
    label->setText(QString("<b>MultiMonitorTools V%1.</b><br>Licensed under the <a href='https://www.gnu.org/licenses/gpl-3.0.en.html'>GPLv3 license</a>.<br>Visit this <a href='https://github.com/ChonkyBuilds/MultiMonitorTools'>github page</a> to check for updates or report bugs.").arg(VERSION));
    label->setWordWrap(false);
    layout->addWidget(label);

    QStringList attributes;
    attributes << "https://www.svgrepo.com/svg/506459/delete-left";

    auto attributesLabel = new QLabel();
    attributesLabel->setTextFormat(Qt::RichText);
    attributesLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    attributesLabel->setOpenExternalLinks(true);

    QString attributeString = "<i>Attributions:</i>";
    for (auto& attribute : attributes)
    {
        attributeString = attributeString + QString("<br><a href='%1'>%1</a>").arg(attribute);
    }

    attributesLabel->setText(attributeString);
    attributesLabel->setWordWrap(false);
    layout->addWidget(attributesLabel);

    auto aboutQtLayout = new QHBoxLayout;
    auto aboutQtButton = new QPushButton("About Qt");
    aboutQtLayout->addWidget(new QLabel("This app is built with Qt6:"));
    aboutQtLayout->addWidget(aboutQtButton);
    aboutQtLayout->addStretch();
    QObject::connect(aboutQtButton, &QPushButton::clicked, &dlg, [=]() {qApp->aboutQt(); });
    layout->addLayout(aboutQtLayout);
    layout->addStretch();
    dlg.exec();
}

void MainWindow::onEditHotkeys()
{
    HotkeyEditor editor;
    editor.exec();
}

void MainWindow::restartCursorAdjust()
{
    if (Settings::instance()->cursorAdjustEnabled())
    {
        qDebug() << "CursorAdjust restarted";
        CursorAdjust::instance()->exit();
        CursorAdjust::instance()->initiate();
    }
}

inline void setForegroundForce(HWND hwnd)
{
    DWORD currentForegroundThreadID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    DWORD myThreadID = GetCurrentThreadId();
    DWORD hwndID = GetWindowThreadProcessId(hwnd, NULL);

    if (currentForegroundThreadID == 0 || myThreadID == currentForegroundThreadID)
    {
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
    }
    else
    {
        AttachThreadInput(myThreadID, currentForegroundThreadID, TRUE);
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        AttachThreadInput(myThreadID, currentForegroundThreadID, FALSE);
    }

    //SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    //SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

void MainWindow::onContextMenu()
{
    if (_contextMenu && _contextMenu->isVisible()) 
    {
        _contextMenu->close();
    }

    _contextMenu = new QMenu();
    _contextMenu->setAttribute(Qt::WA_DeleteOnClose);

    auto menuPoint = QCursor::pos();

    POINT pt;
    pt.x = menuPoint.x();
    pt.y = menuPoint.y();
    
    HWND windowHwnd = GetAncestor(WindowFromPoint(pt), GA_ROOT);
    auto currentDesktop = VirtualDesktop::instance()->currentDesktop();

    if (windowHwnd)
    {
        auto screens = MonitorManager::instance()->screens();
        for (auto& screen : screens)
        {
            _contextMenu->addAction(QString("Move to %1").arg(screen->name()), this, [screen, windowHwnd] { 
                setForegroundForce(windowHwnd);
                ShowWindow(windowHwnd, SW_RESTORE);
                MoveWindow(windowHwnd,
                    screen->physicalCoordinateRect().x(),
                    screen->physicalCoordinateRect().y(),
                    screen->physicalCoordinateRect().width(),
                    screen->physicalCoordinateRect().height(),
                    TRUE);
                ShowWindow(windowHwnd, SW_MAXIMIZE);

                //ShowWindow(hwnd, SW_MAXIMIZE);
                //SetWindowPos(hwnd, HWND_TOP, screen->physicalCoordinateRect().x(), screen->physicalCoordinateRect().y(),
                //screen->physicalCoordinateRect().width(), screen->physicalCoordinateRect().height(),  SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
                //SetWindowPos(hwnd, HWND_TOP,
                //screen->physicalCoordinateRect().x(), screen->physicalCoordinateRect().y(),
                //screen->physicalCoordinateRect().width(),
                //screen->physicalCoordinateRect().height(),
                //SWP_SHOWWINDOW);
                //QTimer::singleShot(500, [hwnd] {ShowWindow(hwnd, SW_MAXIMIZE);});
                //ShowWindow(hwnd, SW_RESTORE); 

                });
        }

        _contextMenu->addSeparator();

        auto desktops = VirtualDesktop::instance()->getDesktops();
        for (auto& desktop : desktops)
        {
            _contextMenu->addAction(VirtualDesktop::instance()->getDesktopName(desktop), this, 
                [this, desktop, windowHwnd, currentDesktop] 
                {
                    setForegroundForce((HWND)_contextMenu->winId());
                    VirtualDesktop::instance()->moveWindowToDesktop(static_cast<void*>(windowHwnd), desktop);

                    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
                    if (modifiers & Qt::ShiftModifier)
                    {
                        VirtualDesktop::instance()->moveToDesktop(desktop);
                    }
                });
        }

    }

    setForegroundForce((HWND)_contextMenu->winId());
    _contextMenu->exec(menuPoint);
    _contextMenu = nullptr;
}

void MainWindow::onHotkeyTriggered(const Hotkey& hotkey)
{
    switch(hotkey.command)
    {
    case Command::SwitchToLeftDesktop:
        onSwitchVirtualDesktop(Direction::Left, false);
        break;
    case Command::SwitchToRightDesktop:
        onSwitchVirtualDesktop(Direction::Right, false);
        break;
    case Command::ShowDesktopName:
        for (auto& overlay : _screenOverlays)
        {
            overlay->showDesktopName();
        }
        break;
    case Command::SwitchToLeftDesktopWithWindow:
        onSwitchVirtualDesktop(Direction::Left, true);
        break;
    case Command::SwitchToRightDesktopWithWindow:
        onSwitchVirtualDesktop(Direction::Right, true);
        break;
    case Command::ContextMenu:
        onContextMenu();
        break;
    default:
        break;
    }
}

}