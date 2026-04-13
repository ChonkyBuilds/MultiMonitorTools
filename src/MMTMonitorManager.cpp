#pragma once
#include "MMTMonitorManager.hpp"

#include <QApplication>
#include <qDebug>
#include <QAbstractNativeEventFilter>
#include <QTimer>
#include <QSettings>
#include <QScreen>

#include <windows.h>
#include <SetupApi.h>

namespace
{

//Code taken from here: https://stackoverflow.com/questions/937044/determine-path-to-registry-key-from-hkey-handle-in-c
std::wstring keyHandleToString(HKEY key)
{
    std::wstring keyPath;
    if (key != NULL)
    {
        HMODULE dll = LoadLibraryW(L"ntdll.dll");
        if (dll != NULL)
        {
            typedef NTSTATUS(__stdcall* NtQueryKeyType)(
                HANDLE  KeyHandle,
                int KeyInformationClass,
                PVOID  KeyInformation,
                ULONG  Length,
                PULONG  ResultLength);

            NtQueryKeyType func = reinterpret_cast<NtQueryKeyType>(::GetProcAddress(dll, "NtQueryKey"));

            if (func != NULL)
            {
                DWORD size = 0;
                DWORD result = 0;
                result = func(key, 3, 0, 0, &size);
                if (result == ((NTSTATUS)0xC0000023L))
                {
                    size = size + 2;
                    wchar_t* buffer = new (std::nothrow) wchar_t[size / sizeof(wchar_t)];
                    if (buffer != NULL)
                    {
                        result = func(key, 3, buffer, size, &size);
                        if (result == ((NTSTATUS)0x00000000L))
                        {
                            buffer[size / sizeof(wchar_t)] = L'\0';
                            keyPath = std::wstring(buffer + 2);
                        }

                        delete[] buffer;
                    }
                }
            }

            FreeLibrary(dll);
        }
    }

    return keyPath;
}


//Adapted from this repo: https://github.com/GKO95/Win32.EDID/blob/master/src/cpp/main.cpp
int getMonitorInfoHelper(QStringList& drvKeys, QStringList& friendlyNames, QStringList& edidHeaders, QList<QSize>& sizes)
{
    DWORD monitorGUIDSize = 0;
    std::vector<GUID> monitorGUIDS;
    SetupDiClassGuidsFromNameW(L"Monitor", NULL, NULL, &monitorGUIDSize);
    monitorGUIDS.resize(monitorGUIDSize);
    BOOL result = SetupDiClassGuidsFromNameW(L"Monitor", monitorGUIDS.data(), monitorGUIDSize, &monitorGUIDSize);
    if (result == FALSE)
    {
        qWarning() << "Unable to retrieve GUIDs of \"Monitor\" class.";
        return 1;
    }

    HDEVINFO deviceInfoSet = INVALID_HANDLE_VALUE;
    deviceInfoSet = SetupDiGetClassDevsW(monitorGUIDS.data(), NULL, NULL, DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        qWarning() << "Unable to retrieve device information from \"Monitor\" class.";
        return 1;
    }

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    DWORD deviceInfoIndex = 0;

    while (SetupDiEnumDeviceInfo(deviceInfoSet, deviceInfoIndex, &deviceInfoData))
    {
        {
            WCHAR friendlyNameBuffer[256];
            DWORD propertyTypes;
            DWORD requiredSizes;
            if (SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, &deviceInfoData, SPDRP_FRIENDLYNAME, &propertyTypes, (BYTE*)friendlyNameBuffer, sizeof(friendlyNameBuffer), &requiredSizes))
            {
                friendlyNames << QString::fromStdWString(std::wstring(friendlyNameBuffer));
            }
            else if (SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC, &propertyTypes, (BYTE*)friendlyNameBuffer, sizeof(friendlyNameBuffer), &requiredSizes))
            {
                friendlyNames << QString::fromStdWString(std::wstring(friendlyNameBuffer));
            }
            else
            {
                friendlyNames << "Generic Monitor";
            }
        }
        
        /*
        THE FUNCTION "SetupDiOpenDevRegKey()" returns handle for the registry key (HKEY) of the
        devDATA included in the device information set devINFO. Device has two registry key:
        hardware (DIREG_DEV) and software (DIREG_DRV) key. EDID can be found in hardware key.

        * DEV: \REGISTRY\MACHINE\SYSTEM\ControlSet001\Enum\DISPLAY\??????\*&********&*&UID****\Device Parameters
        \REGISTRY\MACHINE\SYSTEM\ControlSet001\Enum\DISPLAY\??????\*&********&*&UID****\Device Parameters

        * DRV: \REGISTRY\MACHINE\SYSTEM\ControlSet001\Control\Class\{????????-****-????-****-????????????}\0001
        \REGISTRY\MACHINE\SYSTEM\ControlSet001\Control\Class\{????????-****-????-****-????????????}\0000
        */
        HKEY devKey = SetupDiOpenDevRegKey(deviceInfoSet, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        HKEY drvKey = SetupDiOpenDevRegKey(deviceInfoSet, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
        auto devKeyPath = keyHandleToString(devKey);
        auto drvKeyPath = keyHandleToString(drvKey);

        //old code that manually fetches friendly name from registry
        //{
        //    HKEY tempKey;
        //    WCHAR friendlyNameBuffer[255];
        //    DWORD friendlyNameLength = 255;
        //    unsigned long friendlyNameType = REG_SZ;
        //    auto tempKeyPath = QString::fromStdWString(devKeyPath).replace("\\REGISTRY\\MACHINE\\", "", Qt::CaseInsensitive).replace("\\Device Parameters", "", Qt::CaseInsensitive).toStdWString();
        //    RegOpenKeyW(HKEY_LOCAL_MACHINE, tempKeyPath.c_str(), &tempKey);
        //    RegQueryValueExW(tempKey, L"FriendlyName", NULL, &friendlyNameType, (LPBYTE)&friendlyNameBuffer, &friendlyNameLength);
        //    RegCloseKey(tempKey);
        //
        //    friendlyNames << QString::fromStdWString(std::wstring(friendlyNameBuffer));
        //}

        {
            HKEY tempKey;
            DWORD bufferLength = 0;
            unsigned long type = REG_BINARY;
            auto tempKeyPath = QString::fromStdWString(devKeyPath).replace("\\REGISTRY\\MACHINE\\", "", Qt::CaseInsensitive).toStdWString();
            RegOpenKeyW(HKEY_LOCAL_MACHINE, tempKeyPath.c_str(), &tempKey);
            RegQueryValueExW(tempKey, L"EDID", NULL, &type, NULL, &bufferLength);
            std::vector<BYTE> buffer(bufferLength);
            RegQueryValueExW(tempKey, L"EDID", NULL, &type, buffer.data(), &bufferLength);
            RegCloseKey(tempKey);

            if (bufferLength >= 20)
            {
                QByteArray bytes;
                for (int i = 0; i < 20; i++)
                {
                    bytes.append(buffer[i]);
                }
                edidHeaders << bytes.toHex();
            }
            else
            {
                edidHeaders << "";
            }
            
            if (bufferLength >= 22)
            {
                sizes << QSize(buffer[21], buffer[22]);
            }
            else
            {
                sizes << QSize(0, 0);
            }
        }

        drvKeys << QString::fromStdWString(drvKeyPath).replace("ControlSet001", "CurrentControlSet", Qt::CaseInsensitive).replace("ControlSet002", "CurrentControlSet", Qt::CaseInsensitive);
        deviceInfoIndex++;
    }

    return 0;
}
}

namespace MMT
{

struct MonitorHardwareInfo
{
    QString name;
    QString edidHeader;
    QSize size;
};

struct MonitorInfo
{
    QString name;
    QList<MonitorHardwareInfo> hardwareInfo;
    bool enableVirutalDesktop = false;
    int cursorSpeed = -1;
    QRectF physicalRect;
};

class ScreenImpl : public Screen
{
public:
    ScreenImpl()
    {
    }

    ~ScreenImpl()
    {
    }

public:
    void reset()
    {
        _isValid = false;
        _hardwareInfo.clear();
        _qscreen = nullptr;
        _name = "";
        _virtualDesktopEnabled = false;
        _physicalRect = QRectF();
        _isPrimary = false;
        _physicalCoordinateRect = QRect();
        _logicalCoordinateRect = QRect();
    }

    void initialize(QScreen* screen)
    {
        if (!screen) return;

        _qscreen = screen;
        _name = screen->name();
        _nativeHandle = _qscreen->nativeInterface<QNativeInterface::QWindowsScreen>()->handle();
        MONITORINFOEXW monitorInfoEx;
        monitorInfoEx.cbSize = sizeof(MONITORINFOEXW);
        if (!GetMonitorInfoW(_nativeHandle, &monitorInfoEx))
        {
            return;
        }

        _isPrimary = (monitorInfoEx.dwFlags == MONITORINFOF_PRIMARY);
        _physicalCoordinateRect = QRect(monitorInfoEx.rcMonitor.left, monitorInfoEx.rcMonitor.top, monitorInfoEx.rcMonitor.right-monitorInfoEx.rcMonitor.left, monitorInfoEx.rcMonitor.bottom - monitorInfoEx.rcMonitor.top);
        _logicalCoordinateRect = _qscreen->geometry();

        _isValid = true;
    }

    void appendHardwareInfo(const MonitorHardwareInfo& hardwareInfo)
    {
        _hardwareInfo.append(hardwareInfo);
    }

public:
    QRectF physicalRect() const override
    {
        return _physicalRect;
    }
    void setPhysicalRect(const QRectF& rect) override
    {
        _physicalRect = rect;
    }
    bool virtualDesktopEnabled() const override
    {
        return _virtualDesktopEnabled;
    }
    void setVirtualDesktopEnabled(bool enabled) override
    {
        _virtualDesktopEnabled = enabled;
    }
    QRect physicalCoordinateRect() const override
    {
        return _physicalCoordinateRect;
    }
    QRect logicalCoordinateRect() const override
    {
        return _logicalCoordinateRect;
    }
    QString name() const override
    {
        return _name;
    }
    QStringList hardwareNames() const override
    {
        QStringList result;
        for (auto& info : _hardwareInfo)
        {
            result << info.name;
        }
        return result;
    }
    QStringList hardwareEDIDHeaders() const override
    {
        QStringList result;
        for (auto& info : _hardwareInfo)
        {
            result << info.edidHeader;
        }
        return result;
    }
    bool isValid() const override
    {
        return _isValid;
    }
    QPixmap captureScreenShot()  override
    {
        if (_isValid)
        {
            return _qscreen->grabWindow(0);
        }
        return QPixmap();
    }
    bool isPrimary() const override
    {
        return _isPrimary;
    }
    void* getNativeHandle() override
    {
        return _nativeHandle;
    }

public:
    QList<MonitorHardwareInfo> hardwareInfo() const
    {
        return _hardwareInfo;
    }

    void setHardwareInfo(const QList<MonitorHardwareInfo>& hardwareInfo)
    {
        _hardwareInfo = hardwareInfo;
    }

private:
    QScreen* _qscreen = nullptr;
    bool _isValid = false;
    bool _isPrimary = false;
    QList<MonitorHardwareInfo> _hardwareInfo;
    bool _virtualDesktopEnabled = false;
    QRectF _physicalRect;
    QRect _physicalCoordinateRect;
    QRect _logicalCoordinateRect;
    QString _name;
    HMONITOR _nativeHandle = 0;
};

class DisplayChangeEventFilter : public QObject, public QAbstractNativeEventFilter 
{
public:
    explicit DisplayChangeEventFilter(QObject * parent = 0) : QObject(parent) 
    {
        _timer = new QTimer(this);
        _timer->setSingleShot(true);
        QObject::connect(_timer, &QTimer::timeout, this, &DisplayChangeEventFilter::invokeRefreshMonitorInfo);
    }

    bool nativeEventFilter(const QByteArray& eventType, void* msg,  qintptr* result) override 
    {
        if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") 
        {
            if (msg && static_cast<MSG*>(msg)->message == WM_DISPLAYCHANGE)
            {
                _timer->start(5000);
            }
        } 
        return false;
    };

    void invokeRefreshMonitorInfo()
    {
        QMetaObject::invokeMethod(MonitorManager::instance(), "refreshMonitorInfo");
    }

private:
    QTimer* _timer = nullptr;

};

class MonitorManagerImpl : public MonitorManager
{
public:
    MonitorManagerImpl();
    ~MonitorManagerImpl();

public:
    QList<Screen*> screens() override
    {
        QList<Screen*> result;
        for (auto& screen : _screens)
        {
            if (screen.isValid())
            {
                result.append(static_cast<Screen*>(&screen));
            }
        }
        return result;
    }

public:
    void refreshMonitorInfo() override;
    void syncSettings();

private:
    std::vector<ScreenImpl> _screens;
    DisplayChangeEventFilter* _eventFilter = nullptr;
    QSettings* _monitorSettings = nullptr;
};


MonitorManager* MonitorManager::instance()
{
    static MonitorManagerImpl instance; return &instance;
}

MonitorManagerImpl::MonitorManagerImpl()
{
    _monitorSettings = new QSettings("monitorSettings.ini", QSettings::IniFormat, this);

    _eventFilter = new DisplayChangeEventFilter(this);
    qApp->installNativeEventFilter(_eventFilter);
    refreshMonitorInfo();
}

MonitorManagerImpl::~MonitorManagerImpl()
{
    qApp->removeNativeEventFilter(_eventFilter);
    syncSettings();
}

void MonitorManagerImpl::syncSettings()
{
    for (auto& screen : _screens)
    {
        if (screen.isValid())
        {
            for (auto& monitorHardwareInfo : screen.hardwareInfo())
            {
                _monitorSettings->setValue(monitorHardwareInfo.edidHeader + "/" + "enableVirtualDesktop", screen.virtualDesktopEnabled());
                _monitorSettings->setValue(monitorHardwareInfo.edidHeader + "/" + "physicalRect", screen.physicalRect());
            }
        }
    }
}

void MonitorManagerImpl::refreshMonitorInfo()
{
    auto qscreens = qApp->screens();
    auto primaryqscreen = qApp->primaryScreen();

    double pixelSize = primaryqscreen->physicalSize().width() / primaryqscreen->geometry().width() / primaryqscreen->devicePixelRatio();

    //for (auto s : qscreens)
    //{
    //    qWarning() << s->size();
    //    qWarning() << s->model();
    //    qWarning() << s->manufacturer();
    //    qWarning() << s->name();
    //    qWarning() << s->physicalSize();
    //    qWarning() << s->serialNumber();
    //}

    syncSettings();

    for (auto& screen : _screens)
    {
        screen.reset();
    }

    if (_screens.size() < qscreens.size())
    {
        _screens.resize(qscreens.size());
    }

    QMap<QString, QStringList> displayToMonitorKeysMap;

    {
        DISPLAY_DEVICEW displayDeviceTemp;
        displayDeviceTemp.cb = sizeof(displayDeviceTemp);
        int displayIndex = 0;
        while (EnumDisplayDevicesW(0, displayIndex, &displayDeviceTemp, 0))
        {
            std::wstring currentDisplayNameWstr = displayDeviceTemp.DeviceName;
            QString currentDisplayName = QString::fromStdWString(displayDeviceTemp.DeviceName);
            int monitorIndex = 0;
            while (EnumDisplayDevicesW(currentDisplayNameWstr.c_str(), monitorIndex, &displayDeviceTemp, EDD_GET_DEVICE_INTERFACE_NAME))
            {
                if ((displayDeviceTemp.StateFlags & DISPLAY_DEVICE_ACTIVE) && !(displayDeviceTemp.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
                {
                    displayToMonitorKeysMap[currentDisplayName].append(QString::fromStdWString(displayDeviceTemp.DeviceKey));
                }
                monitorIndex++;
            }
            displayIndex++;
        }
    }

    QStringList drvKeys, friendlyNames, edidHeaders;
    QList<QSize> sizes;
    getMonitorInfoHelper(drvKeys, friendlyNames, edidHeaders, sizes);

    for(int screenIndex = 0; screenIndex < qscreens.size(); screenIndex++)
    {
        bool screenDisplayedOnMonitor = false;

        MONITORINFOEXW monitorInfoEx;
        monitorInfoEx.cbSize = sizeof(MONITORINFOEXW);
        GetMonitorInfoW(qscreens[screenIndex]->nativeInterface<QNativeInterface::QWindowsScreen>()->handle(), &monitorInfoEx);
        auto keys = displayToMonitorKeysMap[QString::fromStdWString(std::wstring(monitorInfoEx.szDevice))];
        for (auto& key : keys)
        {
            for (int i = 0; i < drvKeys.size(); i++)
            {
                if (drvKeys[i].compare(key, Qt::CaseInsensitive) == 0)
                {
                    MonitorHardwareInfo hardwareInfo;
                    hardwareInfo.edidHeader = edidHeaders[i];
                    if (!friendlyNames[i].isNull())
                    {
                        hardwareInfo.name = friendlyNames[i].split(";").last();
                    }
                    hardwareInfo.size = sizes[i];

                    screenDisplayedOnMonitor = true;
                    auto screenGeometry = qscreens[screenIndex]->geometry();
                    _screens[screenIndex].setPhysicalRect(_monitorSettings->value(hardwareInfo.edidHeader + "/" + "physicalRect", QRect(screenGeometry.left()*pixelSize, screenGeometry.top()*pixelSize, qscreens[screenIndex]->physicalSize().width(), qscreens[screenIndex]->physicalSize().height())).value<QRect>());
                    _screens[screenIndex].setVirtualDesktopEnabled(_monitorSettings->value(hardwareInfo.edidHeader + "/" + "enableVirtualDesktop", qscreens[screenIndex] == primaryqscreen).toBool());
                    _screens[screenIndex].appendHardwareInfo(hardwareInfo);
                }
            }
        }

        if (screenDisplayedOnMonitor)
        {
            _screens[screenIndex].initialize(qscreens[screenIndex]);
        }
    }

    emit monitorInfoRefreshed();
    qDebug() << "Monitor info refreshed!";
}

}

