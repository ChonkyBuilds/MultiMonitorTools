#include "MMTHotkeyEditor.hpp"
#include "MMTMonitorManager.hpp"
#include "MMTVirtualDesktop.hpp"
#include "MMTKeyboardHook.hpp"

#include <QLayout>
#include <QScrollArea>
#include <QToolButton>
#include <QPushButton>
#include <QStyle>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QComboBox>
#include <QKeyEvent>
#include <QTimer>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QScrollBar>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


namespace MMT
{

inline QString getKeyName(UINT vkCode) 
{
    if (vkCode == 0)
    {
        return "";
    }

    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    LONG lParam = scanCode << 16;

    switch (vkCode) 
    {
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
    case VK_NEXT: case VK_PRIOR: case VK_LEFT: case VK_RIGHT:
    case VK_UP: case VK_DOWN: case VK_DIVIDE: case VK_RMENU:
    case VK_RCONTROL:
        lParam |= (1 << 24);
        break;
    }

    char name[256];
    if (GetKeyNameTextA(lParam, name, sizeof(name)) != 0) 
    {
        return QString::fromLocal8Bit(name);
    }
    return "Unknown Key";
}

class HotkeyLineEdit : public QLineEdit
{
public:
    HotkeyLineEdit(QWidget* parent = nullptr) : QLineEdit(parent)
    {
    }

    ~HotkeyLineEdit()
    {
    }

public:
    uint8_t triggerKeyCode() const
    {
        return _triggerKeyCode;
    }

    void setTriggerKeyCode(uint8_t keyCode)
    {
        _triggerKeyCode = keyCode;
        setText(getKeyName(_triggerKeyCode));
    }

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        recordKey(event);
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress) 
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            
            //tab controls the GUI instead of a simple keystroke
            if (keyEvent->key() == Qt::Key_Tab)
            {
                recordKey(keyEvent);
                return true;
            }
        }
        return QLineEdit::event(event);
    }

private:
    void recordKey(QKeyEvent* event)
    {
        int key = event->key();

        if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta) 
        {
            return;
        }

        QKeySequence seq(key);
        setText(seq.toString());
        _triggerKeyCode = event->nativeVirtualKey();

        event->accept();
    }

private:
    uint8_t _triggerKeyCode = 0;
};

class HotkeyConfigurator : public QDialog
{
public:
    HotkeyConfigurator(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Hotkey Configurator");

        auto mainLayout = new QVBoxLayout(this);

        auto modifiers = new QGroupBox("Modifiers");
        auto modifierLayout = new QHBoxLayout(modifiers);
        _win = new QCheckBox("Win");
        _shift = new QCheckBox("Shift");
        _alt = new QCheckBox("Alt");
        _control = new QCheckBox("Ctrl");
        modifierLayout->addWidget(_win);
        modifierLayout->addWidget(_shift);
        modifierLayout->addWidget(_alt);
        modifierLayout->addWidget(_control);
        modifierLayout->addStretch();
        mainLayout->addWidget(modifiers);

        _triggerKey = new HotkeyLineEdit;
        auto triggerKeyLayout = new QHBoxLayout;
        _triggerKeyEnabled = new QCheckBox("Trigger key:");
        _triggerKeyEnabled->setChecked(true);
        triggerKeyLayout->addWidget(_triggerKeyEnabled);
        triggerKeyLayout->addWidget(_triggerKey);
        mainLayout->addLayout(triggerKeyLayout);

        _rapidFire = new QCheckBox("Rapid fire hotkey when held");
        _rapidFire->setChecked(false);
        mainLayout->addWidget(_rapidFire);
        _callNextHook = new QCheckBox("Passthrough hotkey to other apps");
        _callNextHook->setChecked(false);
        mainLayout->addWidget(_callNextHook);

        _command = new QComboBox();
        _command->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        for (int i = 1; i < CommandHelper::enumCount(); i++)
        {
            auto description = CommandHelper::getDescription(Command(i));
            if (!description.isEmpty())
            {
                _command->addItem(description, i);
            }
        }

        auto commandLayout = new QHBoxLayout;
        commandLayout->addWidget(new QLabel("Command:"));
        commandLayout->addWidget(_command);
        _monitorSelector = new QComboBox();
        _monitorSelector->setVisible(false);
        commandLayout->addWidget(_monitorSelector);
        _desktopSelector = new QComboBox();
        _desktopSelector->setVisible(false);
        commandLayout->addWidget(_desktopSelector);
        mainLayout->addLayout(commandLayout);
        _command->setCurrentIndex(-1);

        auto desktops = VirtualDesktop::instance()->getDesktops();
        for (auto& desktop : desktops)
        {
            _desktopSelector->addItem(VirtualDesktop::instance()->getDesktopName(desktop));
        }

        auto screens = MonitorManager::instance()->screens();
        for (auto& screen : screens)
        {
            auto edidHeaders = screen->hardwareEDIDHeaders();
            if (edidHeaders.isEmpty())
            {
                qWarning() << "Can't read EDID header for " << screen->name();
            }
            else
            {
                _monitorSelector->addItem(screen->name(), edidHeaders[0]);
            }
        }

        QObject::connect(_command, &QComboBox::currentIndexChanged, this, [this](int index)
            {
                _command->itemData(index);

                switch (Command(_command->itemData(index).toInt()))
                {
                case Command::SwitchToDesktop:
                case Command::SwitchToDesktopWithWindow:
                case Command::MoveWindowToDesktop:
                case Command::MoveWindowToDesktopAndSwitch:
                {
                    _desktopSelector->setVisible(true);
                    _monitorSelector->setVisible(false);
                    break;
                }
                case Command::MoveWindowToMonitor:
                case Command::MoveWindowToMonitorFullscreen:
                {
                    _desktopSelector->setVisible(false);
                    _monitorSelector->setVisible(true);
                    break;
                }
                default:
                {
                    _desktopSelector->setVisible(false);
                    _monitorSelector->setVisible(false);
                    break;
                }
                }
            });

        mainLayout->addStretch();

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        mainLayout->addWidget(buttonBox);

        QObject::connect(_triggerKeyEnabled, &QCheckBox::toggled, this, [this](bool toggled) 
            {
                _triggerKey->setEnabled(toggled);
                _rapidFire->setChecked(!toggled);
                _callNextHook->setChecked(!toggled);
                _rapidFire->setEnabled(toggled);
                _callNextHook->setEnabled(toggled);
            });
    }

    ~HotkeyConfigurator()
    {
    }

public:
    Hotkey hotkey() const
    {
        Hotkey currentHotkey;
        if (_win->isChecked()) currentHotkey.modifier = currentHotkey.modifier | Modifier::Windows;
        if (_alt->isChecked()) currentHotkey.modifier = currentHotkey.modifier | Modifier::Alt;
        if (_shift->isChecked()) currentHotkey.modifier = currentHotkey.modifier | Modifier::Shift;
        if (_control->isChecked()) currentHotkey.modifier = currentHotkey.modifier | Modifier::Control;

        if (_triggerKeyEnabled->isChecked())
        {
            currentHotkey.triggerKey = _triggerKey->triggerKeyCode();
        }
        else
        {
            currentHotkey.triggerKey = 0;
        }

        currentHotkey.command = Command(_command->currentData().toInt());

        currentHotkey.rapidFire = _rapidFire->isChecked();
        currentHotkey.callNextHook = _callNextHook->isChecked();

        switch (currentHotkey.command)
        {
        case Command::SwitchToDesktop:
        case Command::SwitchToDesktopWithWindow:
        case Command::MoveWindowToDesktop:
        case Command::MoveWindowToDesktopAndSwitch:
        {
            currentHotkey.arguments << _desktopSelector->currentText();
            break;
        }
        case Command::MoveWindowToMonitor:
        case Command::MoveWindowToMonitorFullscreen:
        {
            currentHotkey.arguments << _monitorSelector->currentData().toString();

            break;
        }
        default:
        {
            break;
        }
        }

        return currentHotkey;
    }

    void setHotkey(const Hotkey& hotkey)
    {
        _win->setChecked(hotkey.modifier & Modifier::Windows);
        _alt->setChecked(hotkey.modifier & Modifier::Alt);
        _shift->setChecked(hotkey.modifier & Modifier::Shift);
        _control->setChecked(hotkey.modifier & Modifier::Control);

        if (hotkey.triggerKey == 0)
        {
            _triggerKeyEnabled->setChecked(false);
        }
        else
        {
            _triggerKeyEnabled->setChecked(true);
            _triggerKey->setTriggerKeyCode(hotkey.triggerKey);
        }

        _rapidFire->setChecked(hotkey.rapidFire);
        _callNextHook->setChecked(hotkey.callNextHook);

        for (int index = 0; index < _command->count(); index++)
        {        
            if (_command->itemData(index).toInt() == int(hotkey.command))
            {
                _command->setCurrentIndex(index);
            }
        }

        switch (hotkey.command)
        {
        case Command::SwitchToDesktop:
        case Command::SwitchToDesktopWithWindow:
        case Command::MoveWindowToDesktop:
        case Command::MoveWindowToDesktopAndSwitch:
        {
            if (!hotkey.arguments.isEmpty())
            {
                for(int i=0;i<_desktopSelector->count();i++)
                {
                    if (_desktopSelector->itemText(i) == hotkey.arguments[0])
                    {
                        _desktopSelector->setCurrentIndex(i);
                    }
                }
            }
            break;
        }
        case Command::MoveWindowToMonitor:
        case Command::MoveWindowToMonitorFullscreen:
        {
            if (!hotkey.arguments.isEmpty())
            {
                for (int i = 0; i < _monitorSelector->count(); i++)
                {
                    if (_monitorSelector->itemData(i).toString() == hotkey.arguments[0])
                    {
                        _monitorSelector->setCurrentIndex(i);
                    }
                }
            }

            break;
        }
        default:
        {
            break;
        }
        }
    }

private:
    QCheckBox* _win = nullptr;
    QCheckBox* _shift = nullptr;
    QCheckBox* _alt = nullptr;
    QCheckBox* _control = nullptr;
    QCheckBox* _triggerKeyEnabled = nullptr;
    HotkeyLineEdit* _triggerKey = nullptr;
    QComboBox* _command = nullptr;
    QCheckBox* _rapidFire = nullptr;
    QCheckBox* _callNextHook = nullptr;
    QComboBox* _monitorSelector = nullptr;
    QComboBox* _desktopSelector = nullptr;
};

class ToolButton : public QToolButton
{
public:
    ToolButton(QWidget* parent = nullptr) : QToolButton(parent)
    {
    }

    ~ToolButton()
    {
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        int side = height();
        setIconSize(QSize(side - 6, side - 6));
        QToolButton::resizeEvent(event);
    }
};

class HotkeyEditorItem : public QWidget
{
public:
    HotkeyEditorItem(const Hotkey& hotkey, HotkeyEditor* editor, QWidget* parent = nullptr) : QWidget(parent)
    {
        _editor = editor;
        setStyleSheet("QWidget:hover { background-color: #C2C2C2;} QWidget[pressed=\"true\"] { background-color: #DADADA;} QWidget { background-color: #DADADA; border-radius: 5px;}");

        auto mainLayout = new QHBoxLayout(this);
        _label = new QLabel("");
        
        _label->setStyleSheet("QLabel{background-color: transparent}");
        mainLayout->addWidget(_label);
        mainLayout->addStretch();
        _deleteButton = new ToolButton();
        _deleteButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        _deleteButton->setIcon(QIcon(":/resources/delete.svg"));
        _deleteButton->setStyleSheet("QWidget:hover { background-color: #DADADA;border-radius: 5px;} QWidget { background-color: transparent;}");

        mainLayout->addWidget(_deleteButton);

        QObject::connect(_deleteButton, &QToolButton::clicked, this, [this]() {_editor->deleteItem(this); });

        if (hotkey.persistent)
        {
            setEnabled(false);
        }

        setHotkey(hotkey);
    }

    ~HotkeyEditorItem()
    {
    }

public:
    void setHotkey(const Hotkey& hotkey)
    {
        _hotkey = hotkey;
        QString hotkeyString;
        if (_hotkey.modifier & Modifier::Control) hotkeyString = hotkeyString + "Ctrl+";
        if (_hotkey.modifier & Modifier::Alt) hotkeyString = hotkeyString + "Alt+";
        if (_hotkey.modifier & Modifier::Shift) hotkeyString = hotkeyString + "Shift+";
        if (_hotkey.modifier & Modifier::Windows) hotkeyString = hotkeyString + "Win+";
        hotkeyString = hotkeyString + getKeyName(_hotkey.triggerKey);
        if (hotkeyString.endsWith('+')) hotkeyString = hotkeyString.left(hotkeyString.size() - 1);

        QString commandString = CommandHelper::getDescription(_hotkey.command);

        _label->setText(QString("<b><span style='font-size:16pt;'>%1</span></b><br><i><span style='font-size:12pt;'>%2</span></i>").arg(hotkeyString).arg(commandString));
        
    }

    Hotkey hotkey() const
    {
        return _hotkey;
    }

    bool configureHotkey()
    {
        HotkeyConfigurator config;
        config.setHotkey(_hotkey);
        config.exec();

        while (config.result() == QDialog::Accepted)
        {
            bool exist = false;
            for (int i = 0; i < _editor->_listWidget->layout()->count(); i++)
            {
                auto item = dynamic_cast<HotkeyEditorItem*>(_editor->_listWidget->layout()->itemAt(i)->widget());
                if (item && item != this)
                {
                    auto existingHotkey = item->hotkey();
                    if (config.hotkey().modifier == existingHotkey.modifier && config.hotkey().triggerKey == existingHotkey.triggerKey)
                    {
                        exist = true;
                        break;
                    }
                }
            }
            if (exist)
            {
                QMessageBox::warning(_editor, "Hotkey conflict", "Hotkey already exists!");
                config.exec();
            }
            else
            {
                setHotkey(config.hotkey());
                return true;
                break;
            }
        }

        return false;
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        setProperty("pressed", true);
        style()->polish(this);
        QWidget::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        setProperty("pressed", false);
        style()->polish(this);
        QWidget::mouseReleaseEvent(event);

        configureHotkey();
    }

private:
    QLabel* _label = nullptr;
    QToolButton* _deleteButton = nullptr;
    Hotkey _hotkey;
    HotkeyEditor* _editor = nullptr;
};

class HotkeyEditorWidget : public QWidget
{
public:
    HotkeyEditorWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        if (this->parentWidget() && this->parentWidget()->parentWidget()) 
        {
            QScrollArea* area = qobject_cast<QScrollArea*>(this->parentWidget()->parentWidget());
            if (area) 
            {
                area->setMinimumWidth(layout()->sizeHint().width() + area->verticalScrollBar()->width() + area->frameWidth() * 2);
            }
        }
    }
};

HotkeyEditor::HotkeyEditor(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Hotkey Editor");

    auto mainLayout = new QVBoxLayout(this);

    auto scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);

    _listWidget = new HotkeyEditorWidget();
    auto listLayout = new QVBoxLayout(_listWidget);

    for (auto hotkey : HotkeyManager::instance()->hotkeys())
    {
        auto item = new HotkeyEditorItem(hotkey, this);
        listLayout->addWidget(item);
    }

    listLayout->addStretch();

    scrollArea->setWidget(_listWidget);
    mainLayout->addWidget(scrollArea);

    auto buttonsLayout = new QHBoxLayout();
    buttonsLayout->setAlignment(Qt::AlignCenter);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QPushButton* addButton = new QPushButton("New Hotkey");
    addButton->setStyleSheet("padding-left: 10px; padding-right: 10px;padding-top: 4px;padding-bottom: 4px;");

    buttonsLayout->addWidget(addButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(buttonBox);
    mainLayout->addLayout(buttonsLayout);

    QObject::connect(addButton, &QToolButton::clicked, this, [listLayout, this]() 
        {
            auto newItem = new HotkeyEditorItem(Hotkey(), this);
            if (newItem->configureHotkey())
            {
                listLayout->insertWidget(listLayout->count() - 1, newItem);                
            }
            else
            {
                newItem->deleteLater();
            }
        });

    QObject::connect(this, &QDialog::accepted, this, &HotkeyEditor::onAccepted);
}

HotkeyEditor::~HotkeyEditor()
{
}

void HotkeyEditor::onAccepted()
{
    KeyboardHook::instance()->exit();

    HotkeyManager::instance()->removeAllHotkeys();
    for (int i = 0; i < _listWidget->layout()->count(); i++)
    {
        auto item = dynamic_cast<HotkeyEditorItem*>(_listWidget->layout()->itemAt(i)->widget());
        if (item)
        {
            HotkeyManager::instance()->addHotkey(item->hotkey());
        }
    }
    HotkeyManager::instance()->saveHotkeys();

    KeyboardHook::instance()->initiate();
}

void HotkeyEditor::deleteItem(QWidget* item)
{
    auto reply = QMessageBox::question(this, "Delete Hotkey", "Are you sure you want to delete this hotkey?", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) 
    {
        _listWidget->layout()->removeWidget(item);
        item->setParent(nullptr);
        _listWidget->layout()->activate();
        _listWidget->adjustSize();
        item->deleteLater();
    } 
}
}

