#include "VirtualDesktopInterface.hpp"
#include "VirtualDesktop_Win11_23.hpp"
#include "VirtualDesktop_Win11_23H2.hpp"
#include <qDebug>
#include <versionhelpers.h>
#include <Hstring.h>
#include <winstring.h>

template <typename T>
class VirtualDesktopInterfaceWrapper : public VirtualDesktopInterface
{
public:
    VirtualDesktopInterfaceWrapper() {}
    ~VirtualDesktopInterfaceWrapper() {}

    GUID getId(IVirtualDesktop* desktop) override
    {
        return static_cast<T*>(static_cast<IUnknown*>(desktop))->GetID();
    }

    bool isViewVisible(IVirtualDesktop* desktop, IApplicationView* view) override
    {
        BOOL result;
        static_cast<T*>(static_cast<IUnknown*>(desktop))->IsViewVisible(view, &result);
        return result != 0;
    }

    QString getName(IVirtualDesktop* desktop) override
    {
        HSTRING hstr;
        auto result = static_cast<T*>(static_cast<IUnknown*>(desktop))->GetName(&hstr);
        if (result != 0)
        {
            return QString();
        }
        UINT32 length;
        auto stringBuffer = WindowsGetStringRawBuffer(hstr, &length);
        return QString::fromStdWString(std::wstring(stringBuffer, length));
    }
};

template <typename T>
class VirtualDesktopPinnedAppsWrapper : public VirtualDesktopPinnedApps
{
private:
    T* wrapped;
public:
    VirtualDesktopPinnedAppsWrapper(T* w) : wrapped(w) {}
    bool isViewPinned(IApplicationView* view) override
    {
        BOOL result;

        wrapped->IsViewPinned(view, &result);
        return (result == 1);
    }
    int pinView(IApplicationView* view) override
    {
        return wrapped->PinView(view);
    }
    int unpinView(IApplicationView* view) override
    {
        return wrapped->UnpinView(view);
    }
    ~VirtualDesktopPinnedAppsWrapper() {}
};

template <typename T>
class ApplicationViewCollectionWrapper : public ApplicationViewCollection
{
private:
    T* wrapped;
public:
    ApplicationViewCollectionWrapper(T* w) : wrapped(w) {}
    IApplicationView* getViewForHwnd(HWND hwnd) override
    {
        IApplicationView* view;
        wrapped->GetViewForHwnd(hwnd, &view);
        return view;
    }
    ~ApplicationViewCollectionWrapper() {}
};

template <typename T>
class VirtualDesktopManagerInternalWrapper : public VirtualDesktopManagerInternal
{
private:
    T* wrapped;
public:
    VirtualDesktopManagerInternalWrapper(T* w) : wrapped(w) {}
    void moveViewToDesktop(IApplicationView* view, IVirtualDesktop* desktop) override
    {
        wrapped->MoveViewToDesktop(view, desktop);
    }    
    IVirtualDesktop* getCurrentDesktop() override
    {
        IVirtualDesktop* vd;
        wrapped->GetCurrentDesktop(&vd);
        return vd;
    }
    void switchDesktopWithAnimation(IVirtualDesktop* desktop) override
    {
        wrapped->SwitchDesktopWithAnimation(desktop);
    }
    void getLastActiveDesktop(IVirtualDesktop** desktop) override
    {
        wrapped->GetLastActiveDesktop(desktop);
    }
    int getAdjacentDesktop(IVirtualDesktop* from, int direction, IVirtualDesktop** desktop) override
    {
        return wrapped->GetAdjacentDesktop(from, direction, desktop);
    }
    void switchDesktop(IVirtualDesktop* desktop)
    {
        wrapped->SwitchDesktop(desktop);
    }
    ~VirtualDesktopManagerInternalWrapper() {}
};

bool getVirtualDesktopService_Win11_23(VirtualDesktopPinnedApps*& virtualDesktopPinnedApps, ApplicationViewCollection*& applicationViewCollection, VirtualDesktopManagerInternal*& virtualDesktopManagerInternal, VirtualDesktopInterface*& virtualDesktopInterface)
{
    IServiceProvider* serviceProvider;
    HRESULT hr = ::CoCreateInstance(Win11_23::CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IServiceProvider), (PVOID*)&(serviceProvider));

    Win11_23::IVirtualDesktopPinnedApps* pinnedApps = nullptr;
    hr = serviceProvider->QueryService(Win11_23::CLSID_IVirtualDesktopPinnedApps, &pinnedApps);

    virtualDesktopPinnedApps = new VirtualDesktopPinnedAppsWrapper<Win11_23::IVirtualDesktopPinnedApps>(pinnedApps);

    Win11_23::IApplicationViewCollection* applicationViewCollection_11 = nullptr;
    hr = serviceProvider->QueryService(Win11_23::CLSID_IApplicationViewCollection, &applicationViewCollection_11);

    applicationViewCollection = new ApplicationViewCollectionWrapper<Win11_23::IApplicationViewCollection>(applicationViewCollection_11);

    Win11_23::IVirtualDesktopManagerInternal* virtualDesktopManagerInternal_11 = nullptr;
    hr = serviceProvider->QueryService(Win11_23::CLSID_IVirtualDesktopManagerInternal, &virtualDesktopManagerInternal_11);

    virtualDesktopManagerInternal = new VirtualDesktopManagerInternalWrapper<Win11_23::IVirtualDesktopManagerInternal>(virtualDesktopManagerInternal_11);

    virtualDesktopInterface = new VirtualDesktopInterfaceWrapper<Win11_23::IVirtualDesktopInterface>();

    serviceProvider->Release();
    return true;
}

bool getVirtualDesktopService_Win11_23H2(VirtualDesktopPinnedApps*& virtualDesktopPinnedApps, ApplicationViewCollection*& applicationViewCollection, VirtualDesktopManagerInternal*& virtualDesktopManagerInternal, VirtualDesktopInterface*& virtualDesktopInterface)
{
    IServiceProvider* serviceProvider;
    HRESULT hr = ::CoCreateInstance(Win11_23H2::CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IServiceProvider), (PVOID*)&(serviceProvider));

    Win11_23H2::IVirtualDesktopPinnedApps* pinnedApps = nullptr;
    hr = serviceProvider->QueryService(Win11_23H2::CLSID_IVirtualDesktopPinnedApps, &pinnedApps);

    virtualDesktopPinnedApps = new VirtualDesktopPinnedAppsWrapper<Win11_23H2::IVirtualDesktopPinnedApps>(pinnedApps);

    Win11_23H2::IApplicationViewCollection* applicationViewCollection_11 = nullptr;
    hr = serviceProvider->QueryService(Win11_23H2::CLSID_IApplicationViewCollection, &applicationViewCollection_11);

    applicationViewCollection = new ApplicationViewCollectionWrapper<Win11_23H2::IApplicationViewCollection>(applicationViewCollection_11);

    Win11_23H2::IVirtualDesktopManagerInternal* virtualDesktopManagerInternal_11 = nullptr;
    hr = serviceProvider->QueryService(Win11_23H2::CLSID_IVirtualDesktopManagerInternal, &virtualDesktopManagerInternal_11);

    virtualDesktopManagerInternal = new VirtualDesktopManagerInternalWrapper<Win11_23H2::IVirtualDesktopManagerInternal>(virtualDesktopManagerInternal_11);

    virtualDesktopInterface = new VirtualDesktopInterfaceWrapper<Win11_23H2::IVirtualDesktopInterface>();

    serviceProvider->Release();
    return true;
}

int getBuildNumberFromRegistry()
{
    HKEY key;
    WCHAR value[255];
    DWORD value_length = 255;
    unsigned long type = REG_SZ;
    RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", &key);
    RegQueryValueExW(key, L"CurrentBuildNumber", NULL, &type, (LPBYTE)&value, &value_length);
    RegCloseKey(key);

    return std::stoi(std::wstring(value));
}

int getUpdateBuildVersionFromRegistry()
{
    HKEY hKey;
    DWORD buffer;
    LONG result;
    unsigned long type = REG_DWORD, size = 1024;
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey);
    RegQueryValueExW(hKey, L"UBR", NULL, &type, (LPBYTE)&buffer, &size);
    RegCloseKey(hKey);
    return buffer;
}

bool getVirtualDesktopService(VirtualDesktopPinnedApps*& virtualDesktopPinnedApps, ApplicationViewCollection*& applicationViewCollection, VirtualDesktopManagerInternal*& virtualDesktopManagerInternal, VirtualDesktopInterface*& virtualDesktopInterface)
{
    qDebug() << "windows version = " << getBuildNumberFromRegistry() << "." << getUpdateBuildVersionFromRegistry();
    return getVirtualDesktopService_Win11_23H2(virtualDesktopPinnedApps, applicationViewCollection, virtualDesktopManagerInternal, virtualDesktopInterface);
}
