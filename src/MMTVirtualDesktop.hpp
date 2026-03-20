#pragma once

#include "MMT.hpp"
#include <QList>

class IVirtualDesktop;
class QString;

namespace MMT{

class Screen;

class VirtualDesktop
{
public:
    static VirtualDesktop* instance();

public:
    virtual void initializeService() = 0;
    virtual void pinWindowsOnScreen(const QList<Screen*>& screens) = 0;
    virtual bool adjacentDesktopAvailable(Direction) = 0;
    virtual void moveToAdjacentDesktop(Direction, bool bringWindowInFocus = false) = 0;
    virtual IVirtualDesktop* currentDesktop() = 0;
    virtual QString getDesktopName(IVirtualDesktop*) = 0;
    virtual QList<IVirtualDesktop*> getDesktops() = 0;
    virtual void moveWindowToDesktop(void* hwnd, IVirtualDesktop* desktop) = 0;
    virtual void moveToDesktop(IVirtualDesktop*) = 0;

public:
    virtual void testFunction() = 0;

protected:
    VirtualDesktop() = default;
    ~VirtualDesktop() = default;
    VirtualDesktop(VirtualDesktop const&) = delete;
    void operator=(VirtualDesktop const&) = delete;
};
}