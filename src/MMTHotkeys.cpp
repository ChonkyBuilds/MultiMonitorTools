#include "MMTHotkeys.hpp"
#include <qDebug>
#include <QSettings>
#include <QFile>

#include <ranges>

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
    void removeAllHotkeys() override;

private:
    std::map<std::pair<Modifier, uint8_t>, Hotkey> _hotkeys;
};

void HotkeyManagerImpl::loadHotkeys()
{
    _hotkeys.clear();
    //persistent hotkeys
    addHotkey(Hotkey(Modifier::Control | Modifier::Windows, VK_LEFT, false, false, Command::SwitchToLeftDesktop, QStringList(), true));
    addHotkey(Hotkey(Modifier::Control | Modifier::Windows, VK_RIGHT, false, false, Command::SwitchToRightDesktop, QStringList(), true));
    addHotkey(Hotkey(Modifier::Control | Modifier::Windows | Modifier::Shift, VK_RIGHT, false, false, Command::SwitchToRightDesktopWithWindow, QStringList(), true));
    addHotkey(Hotkey(Modifier::Control | Modifier::Windows | Modifier::Shift, VK_LEFT, false, false, Command::SwitchToLeftDesktopWithWindow, QStringList(), true));

    QSettings hotkeySettings("MMTHotkeys.ini", QSettings::IniFormat);
    if (!QFile::exists(hotkeySettings.fileName()))
    {
        //default hotkeys
        addHotkey(Hotkey(Modifier::Control | Modifier::Windows, VK_TAB, false, true, Command::ContextMenu, QStringList()));
        addHotkey(Hotkey(Modifier::Windows | Modifier::Alt, 0, true, false, Command::ShowDesktopName, QStringList()));
    }
    else
    {
        int size = hotkeySettings.beginReadArray("Hotkeys");
        for (int i = 0; i < size; i++)
        {
            hotkeySettings.setArrayIndex(i);
            Hotkey currentHotkey;
            currentHotkey.modifier = Modifier(hotkeySettings.value("modifier").toInt());
            currentHotkey.triggerKey = uint8_t(hotkeySettings.value("triggerKey").toInt());
            currentHotkey.callNextHook = hotkeySettings.value("callNextHook").toBool();
            currentHotkey.rapidFire = hotkeySettings.value("rapidFire").toBool();
            currentHotkey.command = Command(hotkeySettings.value("command").toInt());
            currentHotkey.arguments = hotkeySettings.value("arguments").toStringList();
            
            addHotkey(currentHotkey);
        }
        hotkeySettings.endArray();
    }
}

void HotkeyManagerImpl::saveHotkeys() const
{
    QSettings hotkeySettings("MMTHotkeys.ini", QSettings::IniFormat);
    hotkeySettings.clear();

    hotkeySettings.beginWriteArray("Hotkeys");
    int index = 0;
    for (const auto& [key, hotkey] : _hotkeys) 
    {
        if (!hotkey.persistent)
        {
            hotkeySettings.setArrayIndex(index);
            hotkeySettings.setValue("modifier", int(hotkey.modifier));
            hotkeySettings.setValue("triggerKey", int(hotkey.triggerKey));
            hotkeySettings.setValue("callNextHook", hotkey.callNextHook);
            hotkeySettings.setValue("rapidFire", hotkey.rapidFire);
            hotkeySettings.setValue("command", int(hotkey.command));
            hotkeySettings.setValue("arguments", hotkey.arguments);
            index++;
        }
    }
    hotkeySettings.endArray();
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
    auto values_view = _hotkeys | std::views::values;
    std::vector<Hotkey> values(values_view.begin(), values_view.end());
    return values;
}

bool HotkeyManagerImpl::addHotkey(const Hotkey& hotkey)
{
    auto iter = _hotkeys.find(std::pair<Modifier, uint8_t>(hotkey.modifier, hotkey.triggerKey));
    if (iter != _hotkeys.end())
    {
        qWarning() << "HotkeyManager::addHotkey: hotkey already exists!";
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
        qWarning() << "HotkeyManager::removeHotkey: hotkey does not exist";
        return false;
    }
    if (iter->second.persistent)
    {
        qWarning() << "HotkeyManager::removeHotkey: hotkey cannot be removed!";
        return false;
    }
    _hotkeys.erase(std::pair<Modifier, uint8_t>(hotkey.modifier, hotkey.triggerKey));
    return true;
}

void HotkeyManagerImpl::removeAllHotkeys()
{
    _hotkeys.clear();
}

HotkeyManager* HotkeyManager::instance()
{
    static HotkeyManagerImpl instance;
    return &instance;
}

QString CommandHelper::getDescription(Command command)
{
    switch (command)
    {
    case Command::Undefined:
        return "Undefined";
        break;
    case Command::SwitchToDesktop:
        return "Switch to desktop";
        break;
    case Command::SwitchToDesktopWithWindow:
        return "Switch to desktop and bring the current window";
        break;
    case Command::SwitchToLeftDesktop:
        return "Switch to the desktop on the left";
        break;
    case Command::SwitchToLeftDesktopWithWindow:
        return "Switch to the desktop on the left and bring the current window";
        break;
    case Command::SwitchToRightDesktop:
        return "Switch to the desktop on the right";
        break;
    case Command::SwitchToRightDesktopWithWindow:
        return "Switch to the desktop on the right and bring the current window";
        break;
    case Command::ShowDesktopName:
        return "Show desktop name";
        break;
    case Command::MoveWindowToMonitor:
        return "Move window in focus to the target monitor";
        break;
    case Command::MoveWindowToDesktop:
        return "Move window in focus to the target desktop";
        break;
    case Command::MoveWindowToMonitorContextMenu:
        return "Summon window management context menu(move between monitors)";
        break;
    case Command::MoveWindowToDesktopContextMenu:
        return "Summon window management context menu(move between desktops)";
        break;
    case Command::ContextMenu:
        return "Summon window management context menu(move between monitors and desktops)";
        break;
    defualt:
        return QString();
    }
    return QString();
}

int CommandHelper::enumCount()
{
    return 13;
}


}