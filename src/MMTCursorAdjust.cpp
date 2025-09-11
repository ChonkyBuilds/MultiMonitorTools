#include "MMTCursorAdjust.hpp"
#include "MMTMonitorManager.hpp"

#include <qDebug>
#include <QThread>
#include <QApplication>
#include <QScreen>
#include <omp.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef min
#undef max
namespace MMT
{

struct ScreenBorder
{
    enum Direction
    {
        Top = 0,
        Bottom,
        Left,
        Right,
    };

    ScreenBorder(Direction dir, float start, float end, int target)
    {
        direction = dir;
        startCoordinate = start;
        endcoordinate = end;
        targetScreen = target;
    }

    Direction direction;
    float startCoordinate;
    float endcoordinate;
    int targetScreen;
};

//global variables
static QList<QList<ScreenBorder>> s_borders;
static QList<QRect> s_screenRects;
static QList<QRectF> s_screenPhysicalRects;
static int s_previousScreenNumber = -1;
static double s_cursorAdjustmentRetryThreshold = 10.0;
static int s_cursorAdjustAttemptLimit = 10;

inline int getMonitorNumber(const QList<QRect>& monitors, int x, int y, int padding)
{
    for (int i = 0; i < monitors.size(); i++)
    {
        if (x >= monitors[i].left() + padding && x <= monitors[i].right() - padding && y >= monitors[i].top() + padding && y <= monitors[i].bottom() - padding)
        {
            return i;
        }
    }
    return -1;
}

LRESULT CALLBACK MouseEvent(int nCode, WPARAM wParam, LPARAM lParam)
{
    static int previousxCoord = 0;
    static int previousyCoord = 0;
    static int s_adjustedX = 0;
    static int s_adjustedY = 0;
    static bool s_lmbHeld = false;
    static int s_speedX = 0;
    static int s_speedY = 0;
    static bool s_verifyCursorAdjust = false;
    static int s_cursorAdjustAttempts = 0;

    if (nCode < 0)
    {
        return CallNextHookEx(0, nCode, wParam, lParam);
    }

    if (wParam == WM_LBUTTONDOWN)
    {
        s_lmbHeld = true;
        return CallNextHookEx(0, nCode, wParam, lParam);
    }

    if (wParam == WM_LBUTTONUP)
    {
        s_lmbHeld = false;
        return CallNextHookEx(0, nCode, wParam, lParam);
    }

    if (wParam == WM_MOUSEMOVE)
    {
        int currentCoordX = ((tagMOUSEHOOKSTRUCT*)lParam)->pt.x;
        int currentCoordY = ((tagMOUSEHOOKSTRUCT*)lParam)->pt.y;

        if (s_verifyCursorAdjust)
        {
            double xfactor = std::abs(double(s_adjustedX + s_speedX - currentCoordX) / std::max(std::abs(s_speedX), 1));
            double yfactor = std::abs(double(s_adjustedY + s_speedY - currentCoordY) / std::max(std::abs(s_speedY), 1));

            if ((xfactor > s_cursorAdjustmentRetryThreshold || yfactor > s_cursorAdjustmentRetryThreshold) && (s_cursorAdjustAttempts < s_cursorAdjustAttemptLimit))
            {
                s_cursorAdjustAttempts++;
                //qDebug() << "intervened " << interveneAttempts << " " <<QString("Target: %1:%2; Current: %3:%4; Speed: %5:%6").arg(s_adjustedX).arg(s_adjustedY).arg(xCoord).arg(yCoord).arg(s_speedX).arg(s_speedY);;
                PostThreadMessage( GetCurrentThreadId(), WM_USER, (WPARAM)s_adjustedX, (LPARAM)s_adjustedY );
                return CallNextHookEx(0, nCode, wParam, lParam);
            }
            else
            {
                s_cursorAdjustAttempts = 0;
                s_verifyCursorAdjust = false;
            }
            //qDebug() << QString("Diff: %1:%2 ; Speed: %3:%4").arg(double(previousxCoord + s_speedX - xCoord)/std::max(std::abs(s_speedX), 1)).arg(double(previousyCoord + s_speedY- yCoord)/std::max(std::abs(s_speedY), 1)).arg(s_speedX).arg(s_speedY);
            //qDebug() << QString("Diff: %1:%2 ; Speed: %3:%4").arg(double(s_adjustedX - xCoord)/std::max(std::abs(0), 1)).arg(double(s_adjustedY- yCoord)/std::max(std::abs(0), 1)).arg(s_speedX).arg(s_speedY);
            return CallNextHookEx(0, nCode, wParam, lParam);
        }

        int padding = 0;
        if (s_lmbHeld)
        {
            padding = 2;
        }

        int currentMonitorNumber = getMonitorNumber(s_screenRects, currentCoordX, currentCoordY, padding);
        if (currentMonitorNumber >= 0)
        {
            if (currentMonitorNumber != s_previousScreenNumber)
            {
                int switchFrom = s_previousScreenNumber;
                int switchTo = currentMonitorNumber;

                int predictedSwitchTo = -1;
                bool horizontalSwitch = true;
                for (auto& border : s_borders[switchFrom])
                {
                    if (border.direction == ScreenBorder::Top)
                    {
                        if (previousxCoord >= border.startCoordinate && previousxCoord <= border.endcoordinate && currentCoordY < s_screenRects[switchFrom].top())
                        {
                            predictedSwitchTo = border.targetScreen;
                            horizontalSwitch = false;
                            break;
                        }
                    }
                    if (border.direction == ScreenBorder::Bottom)
                    {
                        if (previousxCoord >= border.startCoordinate && previousxCoord <= border.endcoordinate && currentCoordY > s_screenRects[switchFrom].bottom())
                        {
                            predictedSwitchTo = border.targetScreen;
                            horizontalSwitch = false;
                            break;
                        }
                    }
                    if (border.direction == ScreenBorder::Left)
                    {
                        if (previousyCoord >= border.startCoordinate && previousyCoord <= border.endcoordinate && currentCoordX < s_screenRects[switchFrom].left())
                        {
                            predictedSwitchTo = border.targetScreen;
                            break;
                        }
                    }
                    if (border.direction == ScreenBorder::Right)
                    {
                        if (previousyCoord >= border.startCoordinate && previousyCoord <= border.endcoordinate && currentCoordX > s_screenRects[switchFrom].right())
                        {
                            predictedSwitchTo = border.targetScreen;
                            break;
                        }
                    }
                }

                if (predictedSwitchTo != -1)
                {
                    switchTo = predictedSwitchTo;
                    currentMonitorNumber = predictedSwitchTo;

                }
                else
                {
                    qWarning() << QString("CursorAdjust failure: previous: (%1,%2), current: (%3,%4), switch from: %5, switch to :%6").arg(previousxCoord).arg(previousyCoord).arg(currentCoordX).arg(currentCoordY).arg(switchFrom).arg(switchTo);
                }

                if (!horizontalSwitch)
                {
                    double physicalx = (previousxCoord - s_screenRects[switchFrom].left()) * (s_screenPhysicalRects[switchFrom].width() / s_screenRects[switchFrom].width()) + s_screenPhysicalRects[switchFrom].left();
                    s_adjustedX = (physicalx - s_screenPhysicalRects[switchTo].left()) * (s_screenRects[switchTo].width() / s_screenPhysicalRects[switchTo].width()) + s_screenRects[switchTo].left();
                    s_adjustedY = currentCoordY;
                }
                else
                {
                    double physicaly = (previousyCoord - s_screenRects[switchFrom].top()) * (s_screenPhysicalRects[switchFrom].height() / s_screenRects[switchFrom].height()) + s_screenPhysicalRects[switchFrom].top();
                    s_adjustedX = currentCoordX;
                    s_adjustedY = (physicaly - s_screenPhysicalRects[switchTo].top()) * (s_screenRects[switchTo].height() / s_screenPhysicalRects[switchTo].height()) + s_screenRects[switchTo].top();
                }

                if (s_adjustedX < s_screenRects[switchTo].left())
                {
                    s_adjustedX = s_screenRects[switchTo].left();
                }

                if (s_adjustedX > s_screenRects[switchTo].right() - 1)
                {
                    s_adjustedX = s_screenRects[switchTo].right() - 1;
                }

                if (s_adjustedY < s_screenRects[switchTo].top())
                {
                    s_adjustedY = s_screenRects[switchTo].top();
                }

                if (s_adjustedY > s_screenRects[switchTo].bottom() - 1)
                {
                    s_adjustedY = s_screenRects[switchTo].bottom() - 1;
                }
               
                if (!s_lmbHeld)
                {
                    s_verifyCursorAdjust = true;
                    PostThreadMessage( GetCurrentThreadId(), WM_USER, (WPARAM)s_adjustedX, (LPARAM)s_adjustedY);
                }

                s_previousScreenNumber = currentMonitorNumber;
                return CallNextHookEx(0, nCode, wParam, lParam);
            }
            else
            {
                s_speedX = currentCoordX - previousxCoord;
                s_speedY = currentCoordY - previousyCoord;
                previousxCoord = currentCoordX;
                previousyCoord = currentCoordY;
            }
            s_previousScreenNumber = currentMonitorNumber;
        }
        else
        {
            return CallNextHookEx(0, nCode, wParam, lParam);
        }
    }

    return CallNextHookEx(0, nCode, wParam, lParam);
}

inline bool fuzzyEqual(qreal a, qreal b)
{
    return std::fabs(a - b) < 1;    
}

inline std::array<qreal, 2> computeIntersection(qreal interval1Start, qreal interval1End, qreal interval2Start, qreal interval2End)
{
    std::array<qreal, 2> result = { 0,0 };

    result[0] = qMax(interval1Start, interval2Start);
    result[1] = qMin(interval1End, interval2End);

    if (result[0] >= result[1])
    {
        result[0] = 0;
        result[1] = 0;
    }

    return result;
}

inline ScreenBorder calculateBorder(Screen* mainScreen, Screen* targetScreen)
{
    auto mainScreenPhysicalRect = mainScreen->physicalRect();
    auto targetScreenPhysicalRect = targetScreen->physicalRect();

    ScreenBorder border(ScreenBorder::Left, 0,0, -1);

    if (fuzzyEqual(mainScreenPhysicalRect.left(), targetScreenPhysicalRect.right()))
    {
        border.direction = ScreenBorder::Left;
        auto borderTransitionPhysical = computeIntersection(mainScreenPhysicalRect.top(), mainScreenPhysicalRect.bottom(), targetScreenPhysicalRect.top(), targetScreenPhysicalRect.bottom());
        if (borderTransitionPhysical[1] > borderTransitionPhysical[0])
        {
            border.startCoordinate = mainScreen->physicalCoordinateRect().top();
            border.endcoordinate = mainScreen->physicalCoordinateRect().top() + mainScreen->physicalCoordinateRect().height();
        }

    }
    else if (fuzzyEqual(mainScreenPhysicalRect.right(), targetScreenPhysicalRect.left()))
    {
        border.direction = ScreenBorder::Right;
        auto borderTransitionPhysical = computeIntersection(mainScreenPhysicalRect.top(), mainScreenPhysicalRect.bottom(), targetScreenPhysicalRect.top(), targetScreenPhysicalRect.bottom());
        if (borderTransitionPhysical[1] > borderTransitionPhysical[0])
        {
            border.startCoordinate = mainScreen->physicalCoordinateRect().top();
            border.endcoordinate = mainScreen->physicalCoordinateRect().top() + mainScreen->physicalCoordinateRect().height();
        }
    }
    else if (fuzzyEqual(mainScreenPhysicalRect.top(), targetScreenPhysicalRect.bottom()))
    {
        border.direction = ScreenBorder::Top;
        auto borderTransitionPhysical = computeIntersection(mainScreenPhysicalRect.left(), mainScreenPhysicalRect.right(), targetScreenPhysicalRect.left(), targetScreenPhysicalRect.right());
        if (borderTransitionPhysical[1] > borderTransitionPhysical[0])
        {
            border.startCoordinate = mainScreen->physicalCoordinateRect().left();
            border.endcoordinate = mainScreen->physicalCoordinateRect().left() + mainScreen->physicalCoordinateRect().width();
        }
    }
    else if (fuzzyEqual(mainScreenPhysicalRect.bottom(), targetScreenPhysicalRect.top()))
    {
        border.direction = ScreenBorder::Bottom;
        auto borderTransitionPhysical = computeIntersection(mainScreenPhysicalRect.left(), mainScreenPhysicalRect.right(), targetScreenPhysicalRect.left(), targetScreenPhysicalRect.right());
        if (borderTransitionPhysical[1] > borderTransitionPhysical[0])
        {
            border.startCoordinate = mainScreen->physicalCoordinateRect().left();
            border.endcoordinate = mainScreen->physicalCoordinateRect().left() + mainScreen->physicalCoordinateRect().width();
        }
    }
    return border;
}

class CursorAdjustMouseHook : public CursorAdjust
{
public:
    CursorAdjustMouseHook();
    ~CursorAdjustMouseHook();

public:
    void run()
    {
        s_screenRects.clear();
        s_screenPhysicalRects.clear();

        auto screens = MonitorManager::instance()->screens();
        for (auto& screen : screens)
        {
            s_screenRects << screen->physicalCoordinateRect();
            s_screenPhysicalRects << screen->physicalRect();
            //qDebug() << "screenRect:" << screenRect;
            //qDebug() << "physicalSize:" << screen->physicalSize();
            //qDebug() << "physicalSize(set)" << MonitorManager::instance()->monitorPhysicalRect(screen);
            //qDebug() << "manufact" << screen->manufacturer();
            //qDebug() << "model" << screen->model();
            //qDebug() << "name" << screen->name();
            //qDebug() << "serialNumber" << screen->serialNumber();
        }

        s_borders.clear();

        for (int index = 0; index < screens.size(); index++)
        {
            QList<ScreenBorder> borderList;
            for (int adjacentIndex = 0; adjacentIndex < screens.size(); adjacentIndex++)
            {
                if (adjacentIndex == index) continue;
                auto border = calculateBorder(screens[index], screens[adjacentIndex]);

                if (border.startCoordinate < border.endcoordinate)
                {
                    border.targetScreen = adjacentIndex;
                    borderList << border;
                    //qDebug() << index << ":" << border.direction << ":" << border.startCoordinate << ":" << border.endcoordinate << ":" << border.targetScreen;
                }
            }
            s_borders.append(borderList);
        }


        POINT currentCursorPos;
        GetPhysicalCursorPos(&currentCursorPos);
        s_previousScreenNumber = getMonitorNumber(s_screenRects, currentCursorPos.x, currentCursorPos.y, 0);

        auto mouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)MouseEvent, NULL, 0);

        MSG msg;
        LPARAM lparam;
        WPARAM wparam;
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            if(msg.hwnd == NULL) 
            {
                if( msg.message == WM_USER ) 
                {
                    SetPhysicalCursorPos( (int)msg.wParam, (int)msg.lParam );
                }
            } 
            else 
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        UnhookWindowsHookEx(mouseHook);
        _isRunning = false;
    }

public:
    void initiate() override;
    void exit() override;

private:
    std::thread _thread;
    bool _isRunning = false;
    std::thread _setThread;
};

CursorAdjustMouseHook::CursorAdjustMouseHook()
{
}

CursorAdjustMouseHook::~CursorAdjustMouseHook()
{
    exit();
}

void CursorAdjustMouseHook::initiate()
{
    if (_isRunning)
    {
        return;
    }
    _isRunning = true;
    _thread = std::thread(&CursorAdjustMouseHook::run, this);
}

void CursorAdjustMouseHook::exit()
{
    if (!_isRunning)
    {
        return;
    }

    auto threadId = GetThreadId(_thread.native_handle());
    if (threadId == 0)
    {
        return;
    }

    PostThreadMessage(threadId, WM_QUIT, 0, 0);

    if (_thread.joinable())
    {
        _thread.join();
    }
}

CursorAdjust* CursorAdjust::instance()
{
    static CursorAdjustMouseHook instance;
    return &instance;
}

}
