#pragma once

#include "MMT.hpp"
#include <QStringList>
#include <vector>
#include <type_traits>

namespace MMT
{

enum class Command
{
    Undefined = 0,
    SwitchToDesktop,
    SwitchToDesktopWithWindow,
    SwitchToLeftDesktop,
    SwitchToLeftDesktopWithWindow,
    SwitchToRightDesktop,
    SwitchToRightDesktopWithWindow,
    ShowDesktopName,
    MoveWindowToMonitor,
    MoveWindowToMonitorFullscreen,
    MoveWindowToDesktop,
    MoveWindowToDesktopAndSwitch,
    MoveWindowToMonitorContextMenu,
    MoveWindowToDesktopContextMenu,
    ContextMenu
};

class CommandHelper
{
public:
    static QString getDescription(Command);
    static int enumCount();
};

enum class Modifier : uint8_t
{
    None    = 0,
    Control = 1 << 0,
    Alt     = 1 << 1,
    Shift   = 1 << 2,
    Windows = 1 << 3
};

inline Modifier operator|(Modifier lhs, Modifier rhs) 
{
    using T = std::underlying_type_t<Modifier>;
    return static_cast<Modifier>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline bool operator&(Modifier lhs, Modifier rhs) 
{
    using T = std::underlying_type_t<Modifier>;
    return static_cast<T>(lhs) & static_cast<T>(rhs);
}

struct Hotkey
{
    Modifier modifier = Modifier::None;
    uint8_t triggerKey = 0x00;
    bool callNextHook = false;
    bool rapidFire = false;
    Command command = Command::Undefined;
    QStringList arguments;
    bool persistent = false;

    Hotkey()
    {
    }

    Hotkey(Modifier m, uint8_t tk, bool cnh, bool rf, Command c, const QStringList& arg, bool p = false) : modifier(m), triggerKey(tk), callNextHook(cnh), rapidFire(rf), command(c), arguments(arg), persistent(p)
    {
    }
};

class HotkeyManager
{
public:
    static HotkeyManager* instance();

public:
    virtual void loadHotkeys() = 0;
    virtual void saveHotkeys() const = 0;
    virtual bool findHotkey(Hotkey& result) const = 0;
    virtual std::vector<Hotkey> hotkeys() const = 0;
    virtual bool addHotkey(const Hotkey&) = 0;
    virtual bool removeHotkey(const Hotkey&) = 0;
    virtual void removeAllHotkeys() = 0;

protected:
    HotkeyManager() = default;
    ~HotkeyManager() = default;
    HotkeyManager(HotkeyManager const&) = delete;
    void operator=(HotkeyManager const&) = delete;
};

}