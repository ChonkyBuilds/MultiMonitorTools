#pragma once

#include "MMTVirtualDesktop.hpp"

#include "VirtualDesktopInterface.hpp"
#include "MMTMonitorManager.hpp"
#include "VirtualDesktopGlobals.hpp"

#include <qDebug>
#include <QThread>
#include <objbase.h>
#include <ObjectArray.h>
#include <QRect>
#include <QApplication>
#include <QTimer>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
//#pragma comment(lib, "Ole32.lib")

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ShObjIdl.h>
#include <atlbase.h>

namespace
{

//BOOL IsAltTabWindow(HWND hwnd)
//{
//    TITLEBARINFO ti;
//    HWND hwndTry, hwndWalk = NULL;
//
//    if (!IsWindowVisible(hwnd))
//        return FALSE;
//
//    hwndTry = GetAncestor(hwnd, GA_ROOTOWNER);
//    while (hwndTry != hwndWalk)
//    {
//        hwndWalk = hwndTry;
//        hwndTry = GetLastActivePopup(hwndWalk);
//        if (IsWindowVisible(hwndTry))
//            break;
//    }
//    if (hwndWalk != hwnd)
//        return FALSE;
//
//    // the following removes some task tray programs and "Program Manager"
//    ti.cbSize = sizeof(ti);
//    GetTitleBarInfo(hwnd, &ti);
//    if (ti.rgstate[0] & STATE_SYSTEM_INVISIBLE)
//        return FALSE;
//
//    // Tool windows should not be displayed either, these do not appear in the
//    // task bar.
//    if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
//        return FALSE;
//
//    return TRUE;
//}

bool VisibleInTaskSwitcher(HWND window) 
{
    HWND root = GetAncestor(window, GA_ROOT);

    if (root == NULL || !IsWindowVisible(root)) {
        return false;
    }

    long style = GetWindowLong(root, GWL_EXSTYLE);

    if (style & WS_EX_APPWINDOW) {
        return true;
    }

    if (style & WS_EX_TOOLWINDOW) {
        return false;
    }

    if (style & WS_EX_NOREDIRECTIONBITMAP)
    {
        //return false;
    }

    //LONG lStyle = GetWindowLongPtrW(window, GWL_STYLE);
    //
    //if ((lStyle & WS_VISIBLE) && (lStyle & WS_SYSMENU))
    //{
    //}

    INT nCloaked;
    DwmGetWindowAttribute(window, DWMWA_CLOAKED, &nCloaked, sizeof(nCloaked));
    if (nCloaked)
    {
        return false;
    }

    return true;
}

BOOL IsAltTabWindow(HWND hwnd)
{
    // Start at the root owner
    HWND hwndWalk = GetAncestor(hwnd, GA_ROOTOWNER);
    // See if we are the last active visible popup
    HWND hwndTry;
    while ((hwndTry = GetLastActivePopup(hwndWalk)) != hwndTry) {
        if (IsWindowVisible(hwndTry)) break;
        hwndWalk = hwndTry;
    }
    return hwndWalk == hwnd;
}

std::string ToString(GUID* guid) {
    char guid_string[37]; // 32 hex chars + 4 hyphens + null terminator
    snprintf(
        guid_string, sizeof(guid_string),
        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2],
        guid->Data4[3], guid->Data4[4], guid->Data4[5],
        guid->Data4[6], guid->Data4[7]);
    return guid_string;
}

QString getWindowTitle(HWND hwnd)
{
    int length = GetWindowTextLengthW(hwnd);
    wchar_t* buffer = new wchar_t[length + 1];
    GetWindowTextW(hwnd, buffer, length + 1);
    QString title = QString::fromWCharArray(buffer, length);
    delete[] buffer;
    return title;
}

void forceForegroundWindow(HWND hwnd) {
    DWORD windowThreadProcessId = GetWindowThreadProcessId(GetForegroundWindow(), LPDWORD(0));
    DWORD currentThreadId = GetCurrentThreadId();
    DWORD CONST_SW_SHOW = 5;
    AttachThreadInput(windowThreadProcessId, currentThreadId, true);
    BringWindowToTop(hwnd);
    ShowWindow(hwnd, CONST_SW_SHOW);
    AttachThreadInput(windowThreadProcessId, currentThreadId, false);
}

void outputInfo(HWND hwnd)
{
    auto pwi = new WINDOWINFO;
    pwi->cbSize = sizeof(WINDOWINFO);
    //pwi->cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(hwnd, pwi);

    qDebug() << pwi->rcWindow.top << pwi->rcWindow.bottom << pwi->rcWindow.left << pwi->rcWindow.right << pwi->atomWindowType;
}

inline QString guidToString(GUID guid)
{
    LPOLESTR strGuid = nullptr;
    if (StringFromCLSID(guid, &strGuid))
    {
        auto result = QString::fromWCharArray(strGuid);
        CoTaskMemFree(strGuid);
        return result;
    }
    return QString();
}

}


namespace MMT {

class ThreadAttacher : public QObject
{
public:
    ThreadAttacher(HWND hwnd, QObject* parent = nullptr) : QObject(parent) 
    {
        _hwnd = hwnd;

        DWORD currentThreadId = GetCurrentThreadId();
        DWORD hwndThreadId = GetWindowThreadProcessId(hwnd, LPDWORD(0));

        if (currentThreadId == hwndThreadId)
        {
            deleteLater();
            return;
        }

        if (currentThreadId == 0 || hwndThreadId == 0)
        {
            qWarning() << "attachThreadInput fail: cannot get threadId";
            deleteLater();
            return;
        }

        if (AttachThreadInput(hwndThreadId, currentThreadId, true))
        {
            _connected = true;
        }
        else
        {
            qWarning() << "attachThreadInput fail!" << currentThreadId << hwndThreadId;
            deleteLater();
            return;
        }

        QTimer::singleShot(2500, this, &ThreadAttacher::deleteLater);
    }

    ~ThreadAttacher() 
    {
        if (_connected)
        {
            DWORD currentThreadId = GetCurrentThreadId();
            DWORD hwndThreadId = GetWindowThreadProcessId(_hwnd, LPDWORD(0));
            if (!AttachThreadInput(hwndThreadId, currentThreadId, false))
            {
                qWarning() << "detachThreadInput fail!" << currentThreadId << hwndThreadId;
            }
        }
    }

private:
    HWND _hwnd;
    bool _connected = false;
};

class VirtualDesktopImpl : public VirtualDesktop
{
public:
    VirtualDesktopImpl();
    ~VirtualDesktopImpl();

public:
    void initializeService();
    void pinWindowsOnScreen(const QList<Screen*>& screens);
    bool adjacentDesktopAvailable(Direction direction);
    void moveToAdjacentDesktop(Direction direction, bool bringWindowInFocus);
    IVirtualDesktop* currentDesktop();
    QString getDesktopName(IVirtualDesktop*);
    QList<IVirtualDesktop*> getDesktops();
    void moveWindowToDesktop(void* hwnd, IVirtualDesktop* desktop);
    void moveToDesktop(IVirtualDesktop* targetDesktop, bool bringWindowInFocus);
    IVirtualDesktop* getDesktopByName(const QString& name);

public:
    void testFunction();

private:
    std::unique_ptr<VirtualDesktopPinnedApps> _virtualDesktopPinnedApps = nullptr;
    std::unique_ptr<ApplicationViewCollection> _applicationViewCollection = nullptr;
    std::unique_ptr<VirtualDesktopManagerInternal> _virtualDesktopManagerInternal = nullptr;
    std::unique_ptr<VirtualDesktopInterface> _virtualDesktopInterface = nullptr;
    CComPtr<IVirtualDesktopManager> _virtualDesktopManager = nullptr;
    bool _comInit;
};

VirtualDesktopImpl::VirtualDesktopImpl()
{
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
    {
        _comInit = false;
        qCritical() << "COM initialize failed!";
    }
    else
    {
        _comInit = true;
    }

    initializeService();
}

VirtualDesktopImpl::~VirtualDesktopImpl()
{
    if (_comInit)
    {
        CoUninitialize();
    }
}

void VirtualDesktopImpl::initializeService()
{
    VirtualDesktopPinnedApps* virtualDesktopPinnedApps;
    ApplicationViewCollection* applicationViewCollection;
    VirtualDesktopManagerInternal* virtualDesktopManagerInternal;
    VirtualDesktopInterface* virtualDesktopInterface;
    getVirtualDesktopService(virtualDesktopPinnedApps, applicationViewCollection, virtualDesktopManagerInternal, virtualDesktopInterface);
    _virtualDesktopPinnedApps.reset(virtualDesktopPinnedApps);
    _applicationViewCollection.reset(applicationViewCollection);
    _virtualDesktopManagerInternal.reset(virtualDesktopManagerInternal);
    _virtualDesktopInterface.reset(virtualDesktopInterface);

    if (FAILED(CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&_virtualDesktopManager))))
    {
        qCritical() << "VirtualDesktopManager creation failed!";
    }

}

void VirtualDesktopImpl::testFunction()
{
    return;
    //HWND target = NULL;
    //for (HWND hwnd = GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT))
    //{
    //    if (!IsWindowVisible(hwnd))
    //        continue;
    //    int length = GetWindowTextLength(hwnd);
    //    if (length == 0)
    //        continue;
    //
    //    if (!VisibleInTaskSwitcher(hwnd))
    //    {
    //        continue;
    //    }
    //    auto view = _applicationViewCollection->getViewForHwnd(hwnd);
    //
    //    if (view)
    //    {
    //        if (!_virtualDesktopPinnedApps->isViewPinned(view))
    //        {
    //            if (_virtualDesktopInterface->isViewVisible(_virtualDesktopManagerInternal->getCurrentDesktop(), view))
    //            {
    //                target = hwnd;
    //                break;
    //            }
    //        }
    //    }
    //
    //}
    //qDebug() << "forcing: " << getWindowTitle(target);
    //forceForegroundWindow(target);
    //
    ////qDebug() << toString(target);
    //return;
    ////qDebug() << "cuurent focus 1:" << toString(GetFocus());
    //
    //auto currentDesktop = _virtualDesktopManagerInternal->getCurrentDesktop();
    //int length = GetWindowTextLength(_focusedWindow[currentDesktop]);
    //wchar_t* title = new wchar_t[length + 1];
    //GetWindowTextW(_focusedWindow[currentDesktop], title, length + 1);
    //QString name = QString::fromWCharArray(title, length);
    ////qDebug() << "current focus 2: " << name;
    ////BringWindowToTop(_focusedWindow[currentDesktop]);
    //forceForegroundWindow(_focusedWindow[currentDesktop]);
    //qDebug() << "setForegroundWindow: " << name;
    //SetFocus(_focusedWindow[currentDesktop]);;
    //ShowWindow(_focusedWindow[currentDesktop], 5);

    //qDebug() << "cuurent focus 3:" << toString(GetFocus());

    //qDebug() << "testfunction";
    //return;
    //IServiceProvider* pServiceProvider = nullptr;
    //HRESULT hr = ::CoCreateInstance(CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IServiceProvider), (PVOID*)&pServiceProvider);
    //
    //if (SUCCEEDED(hr))
    //{
    //    IVirtualDesktopManagerInternal2* pDesktopManagerInternal = nullptr;
    //    hr = pServiceProvider->QueryService(CLSID_VirtualDesktopAPI_Unknown, &pDesktopManagerInternal);
    //
    //    if (SUCCEEDED(hr))
    //    {
    //        qDebug() << "success";
    //        UINT haha;
    //        int placement = 0;
    //        //qDebug() << pDesktopManagerInternal->GetCount(&placement, &haha);
    //        //qDebug() << haha;
    //
    //        //IVirtualDesktop* top = nullptr;
    //        //qDebug() << pDesktopManagerInternal->GetCurrentDesktop(&placement, top);
    //    }
    //    else
    //    {
    //        qDebug() << "fail";
    //    }
    //    return;
    //}

}

void VirtualDesktopImpl::pinWindowsOnScreen(const QList<Screen*>& screens)
{
    for (HWND hwnd = GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT))
    {
        if (!IsWindowVisible(hwnd))
            continue;

        int length = GetWindowTextLength(hwnd);
        if (length == 0)
            continue;

        if (!VisibleInTaskSwitcher(hwnd))
        {
            continue;
        }

        auto view = _applicationViewCollection->getViewForHwnd(hwnd);
        if (!view)
        {
            continue;
        }

        auto monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        bool pin = false;

        for (auto& screen : screens)
        {
            if (screen->getNativeHandle() == monitor)
            {
                pin = true;
            }
        }

        if (pin)
        {
            if (!_virtualDesktopPinnedApps->isViewPinned(view))
            {
                _virtualDesktopPinnedApps->pinView(view);
                //qDebug() << "pinned: " << getWindowTitle(hwnd);
            }
        }
        else
        {
            if (_virtualDesktopPinnedApps->isViewPinned(view))
            {
                _virtualDesktopPinnedApps->unpinView(view);
                //qDebug() << "unpinned: " << getWindowTitle(hwnd);
            }
        }
    }
}

bool VirtualDesktopImpl::adjacentDesktopAvailable(Direction direction)
{
    int directionCode = direction == Direction::Left ? 3 : 4;
    IVirtualDesktop* adjacentDesktop;
    _virtualDesktopManagerInternal->getAdjacentDesktop(_virtualDesktopManagerInternal->getCurrentDesktop(), directionCode, &adjacentDesktop);
    return (adjacentDesktop != nullptr);
}

void VirtualDesktopImpl::moveToAdjacentDesktop(Direction direction, bool bringWindowInFocus)
{
    int directionCode = direction == Direction::Left ? 3 : 4;

    IVirtualDesktop* targetDesktop;
    _virtualDesktopManagerInternal->getAdjacentDesktop(_virtualDesktopManagerInternal->getCurrentDesktop(), directionCode, &targetDesktop);
    if (targetDesktop != nullptr)
    {
        moveToDesktop(targetDesktop, bringWindowInFocus);
    }
}

IVirtualDesktop* VirtualDesktopImpl::currentDesktop()
{
    return _virtualDesktopManagerInternal->getCurrentDesktop();
}

QString VirtualDesktopImpl::getDesktopName(IVirtualDesktop* desktop)
{
    if (!desktop)
    {
        return QString();
    }

    auto name = _virtualDesktopInterface->getName(desktop);
    if (name.isEmpty())
    {
        int desktopNumber = 0;
        IVirtualDesktop* targetDesktop = desktop;
        do
        {
            IVirtualDesktop* adjacentDesktop;
            _virtualDesktopManagerInternal->getAdjacentDesktop(targetDesktop, 3, &adjacentDesktop);
            desktopNumber++;
            targetDesktop = adjacentDesktop;
        } while (targetDesktop != nullptr);

        return QString("Desktop %1").arg(desktopNumber);
    }
    return name;
}

QList<IVirtualDesktop*> VirtualDesktopImpl::getDesktops()
{
    QList<IVirtualDesktop*> desktops;
    auto pCurrentDesktop = currentDesktop();
    
    IVirtualDesktop* desktopIter = pCurrentDesktop;
    while(desktopIter)
    {
        _virtualDesktopManagerInternal->getAdjacentDesktop(desktopIter, 3, &desktopIter);
        if (desktopIter)
        {
            desktops.prepend(desktopIter);
        }
    }

    desktops.append(pCurrentDesktop);
    
    desktopIter = pCurrentDesktop;
    while(desktopIter)
    {
        _virtualDesktopManagerInternal->getAdjacentDesktop(desktopIter, 4, &desktopIter);
        if (desktopIter)
        {
            desktops.append(desktopIter);
        }
    }

    return desktops;
}

void VirtualDesktopImpl::moveWindowToDesktop(void* hwnd, IVirtualDesktop* desktop)
{
    if (hwnd && VisibleInTaskSwitcher((HWND)hwnd))
    {
        auto view = _applicationViewCollection->getViewForHwnd((HWND)hwnd);
        if (view && !_virtualDesktopPinnedApps->isViewPinned(view))
        {
            _virtualDesktopManagerInternal->moveViewToDesktop(view, desktop);
        }
    }
}

void VirtualDesktopImpl::moveToDesktop(IVirtualDesktop* targetDesktop, bool bringWindowInFocus)
{
    if (targetDesktop != nullptr)
    {
        auto targetDesktopGuid = _virtualDesktopInterface->getId(targetDesktop);
        auto currentDesktopGuid = _virtualDesktopInterface->getId(_virtualDesktopManagerInternal->getCurrentDesktop());

        HWND hwnd = GetTopWindow(NULL);
        while (hwnd) 
        {    
            GUID desktopId{};
            if(SUCCEEDED(_virtualDesktopManager->GetWindowDesktopId(hwnd, &desktopId)))
            {
                if (desktopId == targetDesktopGuid)
                {
                    new ThreadAttacher(hwnd, qApp);
                    break;
                }
            }
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
        }

        hwnd = GetTopWindow(NULL);
        while (hwnd) 
        {    
            GUID desktopId{};
            if(SUCCEEDED(_virtualDesktopManager->GetWindowDesktopId(hwnd, &desktopId)))
            {
                if (desktopId == currentDesktopGuid)
                {
                    new ThreadAttacher(hwnd, qApp);
                    break;
                }
            }
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
        }

        if (bringWindowInFocus)
        {
            auto currentWindowInFocusHwnd = GetForegroundWindow();
            if (currentWindowInFocusHwnd && VisibleInTaskSwitcher(currentWindowInFocusHwnd))
            {
                auto viewInFocus = _applicationViewCollection->getViewForHwnd(currentWindowInFocusHwnd);
                if (viewInFocus && !_virtualDesktopPinnedApps->isViewPinned(viewInFocus))
                {
                    _virtualDesktopManagerInternal->moveViewToDesktop(viewInFocus, targetDesktop);
                }
            }            
        }

        _virtualDesktopManagerInternal->switchDesktop(targetDesktop);
    }
}

IVirtualDesktop* VirtualDesktopImpl::getDesktopByName(const QString& name)
{
    IVirtualDesktop* targetDesktop = nullptr;

    auto desktops = VirtualDesktop::instance()->getDesktops();
    for (auto& desktop : desktops)
    {
        if (name == VirtualDesktop::instance()->getDesktopName(desktop))
        {
            targetDesktop = desktop;
            break;
        }
    }

    return targetDesktop;
}

VirtualDesktop* VirtualDesktop::instance()
{
    static VirtualDesktopImpl instance;
    return &instance;
}

}