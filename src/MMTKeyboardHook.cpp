#include "MMTKeyboardHook.hpp"
#include "MMTVirtualDesktop.hpp"
#include "MMTHotkeys.hpp"

#include <qDebug>
#include <QThread>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace MMT {

static bool ctrlDown = false;
static bool winDown = false;
static bool shiftDown = false;
static bool altDown = false;

static bool keysDown[256];

LRESULT keyboardHook(const int code, const WPARAM wParam, const LPARAM lParam)
{
    if (code < 0)
    {
        return CallNextHookEx(0, code, wParam, lParam);
    }

    if (wParam == WM_KEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYDOWN || wParam == WM_SYSKEYUP)
    {
        bool keyDown = false;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            keyDown = true;
        }

        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        DWORD keyCode = kbdStruct->vkCode;

        if (keyCode < 0 || keyCode > 255)
        {
            return CallNextHookEx(0, code, wParam, lParam);
        }

        if (keyCode == VK_LCONTROL || keyCode == VK_RCONTROL || keyCode == VK_CONTROL)
        {
            ctrlDown = keyDown;
        }

        if (keyCode == VK_LWIN || keyCode == VK_RWIN)
        {
            winDown = keyDown;
        }
        
        if (keyCode == VK_LSHIFT || keyCode == VK_RSHIFT || keyCode == VK_SHIFT)
        {
            shiftDown = keyDown;
        }

        if (keyCode == VK_MENU || keyCode == VK_LMENU || keyCode == VK_RMENU)
        {
            altDown = keyDown;
        }

        Hotkey noTriggerHotkey;
        if (ctrlDown) noTriggerHotkey.modifier = noTriggerHotkey.modifier | Modifier::Control;
        if (winDown) noTriggerHotkey.modifier = noTriggerHotkey.modifier | Modifier::Windows;
        if (altDown) noTriggerHotkey.modifier = noTriggerHotkey.modifier | Modifier::Alt;
        if (shiftDown) noTriggerHotkey.modifier = noTriggerHotkey.modifier | Modifier::Shift;
        noTriggerHotkey.triggerKey = 0;

        if (HotkeyManager::instance()->findHotkey(noTriggerHotkey))
        {
            emit KeyboardHook::instance()->hotkeyTriggered(noTriggerHotkey);
        }

        if (keyDown)
        {
            Hotkey currentHotkey;
            if (ctrlDown) currentHotkey.modifier = currentHotkey.modifier | Modifier::Control;
            if (winDown) currentHotkey.modifier = currentHotkey.modifier | Modifier::Windows;
            if (altDown) currentHotkey.modifier = currentHotkey.modifier | Modifier::Alt;
            if (shiftDown) currentHotkey.modifier = currentHotkey.modifier | Modifier::Shift;
            currentHotkey.triggerKey = keyCode;

            if (HotkeyManager::instance()->findHotkey(currentHotkey))
            {
                if (currentHotkey.rapidFire || !keysDown[keyCode])
                {
                    emit KeyboardHook::instance()->hotkeyTriggered(currentHotkey);
                }
                keysDown[keyCode] = keyDown;

                if (!currentHotkey.callNextHook)
                {
                    return 1;
                }
            }
        }

        keysDown[keyCode] = keyDown;
    }

    return CallNextHookEx(0, code, wParam, lParam);
}

class KeyboardHookImpl : public KeyboardHook
{
public:
    KeyboardHookImpl()
    {

    }

    ~KeyboardHookImpl()
    {
        exit();
    }

    void run()
    {
        for (int i = 0; i < 256; i++)
        {
            keysDown[i] = GetKeyState(i) < 0;
        }
        ctrlDown = GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0 || GetKeyState(VK_CONTROL) < 0;
        winDown = GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0;
        shiftDown = GetKeyState(VK_LSHIFT) < 0 || GetKeyState(VK_RSHIFT) < 0 || GetKeyState(VK_SHIFT) < 0;
        altDown = GetKeyState(VK_LMENU) < 0 || GetKeyState(VK_RMENU) < 0 || GetKeyState(VK_MENU) < 0;

        auto hook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)keyboardHook, NULL, 0);
        if (hook == NULL) 
        {
            qDebug() << "Keyboard hook failed!";
            return;
        }
        qDebug() << "keyboard hooked!";

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        UnhookWindowsHookEx(hook);
        _isRunning = false;
    }

    void exit() override
    {
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

    void initiate() override
    {
        if (_isRunning)
        {
            return;
        }
        _isRunning = true;
        _thread = std::thread(&KeyboardHookImpl::run, this);
    }

private:
    std::thread _thread;
    bool _isRunning = false;
};

KeyboardHook* KeyboardHook::instance()
{
    static KeyboardHookImpl instance; return &instance;
}

}


