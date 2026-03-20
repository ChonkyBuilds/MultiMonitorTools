#include "MMTKeyboardHook.hpp"
#include "MMTVirtualDesktop.hpp"

#include <qDebug>
#include <QThread>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace MMT {

static bool leftDown = false;
static bool rightDown = false;
static bool ctrlDown = false;
static bool winDown = false;
static bool shiftDown = false;
static bool altDown = false;

static bool tabDown = false;

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

        if (keyCode == VK_LCONTROL || keyCode == VK_RCONTROL)
        {
            ctrlDown = keyDown;
        }

        if (keyCode == VK_LWIN || keyCode == VK_RWIN)
        {
            winDown = keyDown;
        }
        
        if (keyCode == VK_LSHIFT || keyCode == VK_RSHIFT)
        {
            shiftDown = keyDown;
        }

        if (keyCode == VK_MENU || keyCode == VK_LMENU || keyCode == VK_RMENU)
        {
            altDown = keyDown;
        }

        if (keyCode == VK_TAB)
        {
            tabDown = keyDown;
        }

        if (ctrlDown && winDown && tabDown)
        {
            emit KeyboardHook::instance()->contextMenuHotkeyTriggered();
            return 1;
        }

        if (altDown && winDown)
        {
            emit KeyboardHook::instance()->previewDesktopNameTriggered();
        }

        if (keyCode == VK_LEFT)
        {
            if (keyDown && ctrlDown && winDown)
            {
                if (!leftDown)
                {
                    leftDown = true;
                    emit KeyboardHook::instance()->switchVirtualDesktopHotkeyTriggerd(Direction::Left, shiftDown);
                }
                return 1;
            }

            leftDown = keyDown;
        }

        if (keyCode == VK_RIGHT)
        {
            if (keyDown && ctrlDown && winDown)
            {
                if (!rightDown)
                {
                    rightDown = true;
                    emit KeyboardHook::instance()->switchVirtualDesktopHotkeyTriggerd(Direction::Right, shiftDown);
                }
                return 1;
            }

            rightDown = keyDown;
        }
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
        leftDown = GetKeyState(VK_LEFT) < 0;
        rightDown = GetKeyState(VK_RIGHT) < 0;
        ctrlDown = GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_LCONTROL) < 0;
        winDown = GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0;
        shiftDown = GetKeyState(VK_LSHIFT) < 0 || GetKeyState(VK_RSHIFT) < 0;

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


