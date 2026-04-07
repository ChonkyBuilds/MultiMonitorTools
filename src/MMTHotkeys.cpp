#include "MMTHotkeys.hpp"
#include <qDebug>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace MMT
{

class HotkeyManagerImpl : public HotkeyManager
{
public:
    HotkeyManagerImpl()
    {
    }
    ~HotkeyManagerImpl()
    {
    }

public:
    void loadHotkeys() override;
    void saveHotkeys() const override;
    bool findHotkey(Hotkey& result) const override;
    std::vector<Hotkey> hotkeys() const override;
    bool addHotkey(const Hotkey&) override;
    bool removeHotkey(const Hotkey&) override;

private:
    std::map<std::pair<Modifier, uint8_t>, Hotkey> _hotkeys;
};

void HotkeyManagerImpl::loadHotkeys()
{
    _hotkeys.clear();
    addHotkey(Hotkey(Modifier::Control | Modifier::Windows, VK_LEFT, false, false, Command::SwitchToLeftDesktop, QStringList()));
    addHotkey(Hotkey(Modifier::Control | Modifier::Windows, VK_RIGHT, false, false, Command::SwitchToRightDesktop, QStringList()));
    addHotkey(Hotkey(Modifier::Control | Modifier::Windows | Modifier::Shift, VK_RIGHT, false, false, Command::SwitchToRightDesktopWithWindow, QStringList()));
    addHotkey(Hotkey(Modifier::Control | Modifier::Windows | Modifier::Shift, VK_LEFT, false, false, Command::SwitchToLeftDesktopWithWindow, QStringList()));
    addHotkey(Hotkey(Modifier::Control | Modifier::Windows, VK_TAB, false, true, Command::ContextMenu, QStringList()));
    addHotkey(Hotkey(Modifier::Windows | Modifier::Alt, 0, true, false, Command::ShowDesktopName, QStringList()));
}

void HotkeyManagerImpl::saveHotkeys() const
{
}

bool HotkeyManagerImpl::findHotkey(Hotkey& result) const
{
    auto iter = _hotkeys.find(std::pair<Modifier, uint8_t>(result.modifier, result.triggerKey));
    if (iter != _hotkeys.end())
    {
        result = iter->second;
        return true;
    }
    return false;
}

std::vector<Hotkey> HotkeyManagerImpl::hotkeys() const
{
    return std::vector<Hotkey>();
}

bool HotkeyManagerImpl::addHotkey(const Hotkey& hotkey)
{
    auto iter = _hotkeys.find(std::pair<Modifier, uint8_t>(hotkey.modifier, hotkey.triggerKey));
    if (iter != _hotkeys.end())
    {
        qDebug() << "HotkeyManager::addHotkey: hotkey already exists!";
        return false;
    }
    _hotkeys[std::pair<Modifier, uint8_t>(hotkey.modifier, hotkey.triggerKey)] = hotkey;
    return true;
}

bool HotkeyManagerImpl::removeHotkey(const Hotkey& hotkey)
{
    auto iter = _hotkeys.find(std::pair<Modifier, uint8_t>(hotkey.modifier, hotkey.triggerKey));
    if (iter == _hotkeys.end())
    {
        qDebug() << "HotkeyManager::removeHotkey: hotkey does not exist";
        return false;
    }
    _hotkeys.erase(std::pair<Modifier, uint8_t>(hotkey.modifier, hotkey.triggerKey));
    return true;
}

HotkeyManager* HotkeyManager::instance()
{
    static HotkeyManagerImpl instance;
    return &instance;
}

}