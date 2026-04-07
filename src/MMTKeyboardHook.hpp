#pragma once

#include "MMT.hpp"
#include <QObject>

namespace MMT
{

struct Hotkey;

class KeyboardHook : public QObject
{
    Q_OBJECT
public:
    static KeyboardHook* instance();

public slots:
    virtual void initiate() = 0;
    virtual void exit() = 0;

signals:
    void hotkeyTriggered(const Hotkey&);

protected:
    KeyboardHook() = default;
    ~KeyboardHook() = default;
    KeyboardHook(KeyboardHook const&) = delete;
    void operator=(KeyboardHook const&) = delete;
};

}