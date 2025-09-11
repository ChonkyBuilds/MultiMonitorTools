#pragma once
#include "VirtualDesktopGlobals.hpp"
#include <QString>

class VirtualDesktopPinnedApps
{
public:
    virtual bool isViewPinned(IApplicationView* view) = 0;
    virtual int pinView(IApplicationView* view) = 0;
    virtual int unpinView(IApplicationView* view) = 0;
    virtual ~VirtualDesktopPinnedApps() = default;
};

class ApplicationViewCollection
{
public:
    virtual IApplicationView* getViewForHwnd(HWND view) = 0;
    virtual ~ApplicationViewCollection() = default;
};

class VirtualDesktopManagerInternal
{
public:
    virtual void moveViewToDesktop(IApplicationView*, IVirtualDesktop*) = 0;
    virtual IVirtualDesktop* getCurrentDesktop() = 0;
    virtual void switchDesktopWithAnimation(IVirtualDesktop*) = 0;
    virtual void getLastActiveDesktop(IVirtualDesktop**) = 0;
    virtual int getAdjacentDesktop(IVirtualDesktop*, int, IVirtualDesktop**) = 0;
    virtual void switchDesktop(IVirtualDesktop*) = 0;
    virtual ~VirtualDesktopManagerInternal() = default;
};

class VirtualDesktopInterface
{
public:
    virtual bool isViewVisible(IVirtualDesktop*, IApplicationView*) = 0;
    virtual GUID getId(IVirtualDesktop*) = 0;
    virtual QString getName(IVirtualDesktop*) = 0;
    virtual ~VirtualDesktopInterface() = default;
};

bool getVirtualDesktopService(VirtualDesktopPinnedApps*&, ApplicationViewCollection*&, VirtualDesktopManagerInternal*&, VirtualDesktopInterface*&);