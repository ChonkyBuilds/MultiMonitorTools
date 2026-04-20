#include "MMTTrayIconManager.hpp"
#include "MMTSettings.hpp"
#include "MMTVirtualDesktop.hpp"

#include <QLayout>
#include <QCheckBox>
#include <QComboBox>
#include <qDebug>
#include <QTimer>
#include <QToolButton>
#include <QDialog>
#include <QRadioButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QKeyEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace MMT
{

class IconConfigureDialog : public QDialog
{
public:
    IconConfigureDialog(const QString& desktopName, QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle(QString("Configure tray icon for %1").arg(desktopName));
        auto mainLayout = new QVBoxLayout(this);

        auto group1 = new QGroupBox("Display a letter on the default tray icon");
        auto group1Layout = new QHBoxLayout(group1);
        auto letter = new QLineEdit;
        letter->setMaxLength(1);
        group1Layout->addWidget(letter);
        auto setButton = new QPushButton("Set");
        group1Layout->addWidget(setButton);

        QObject::connect(setButton, &QPushButton::clicked, this, [this, desktopName, letter]()
            {
                if (letter->text().isEmpty())
                {
                    QMessageBox::warning(this, "Empty letter", "Please enter a letter.");
                }
                else
                {
                    Settings::instance()->setCustomTrayIconTextForDesktop(desktopName, letter->text().at(0));
                    accept();
                }
            });

        auto group2 = new QGroupBox("Load a custom image");
        auto group2Layout = new QVBoxLayout(group2);
        auto selectImageButton = new QPushButton("Select image");
        group2Layout->addWidget(selectImageButton);

        QObject::connect(selectImageButton, &QPushButton::clicked, this, [this, desktopName]()
            {
                QString fileName = QFileDialog::getOpenFileName(this, "Select Image", "", "Images (*.png *.bmp *.jpg *.jpeg)");
                if (!fileName.isEmpty())
                {
                    Settings::instance()->setCustomTrayIconPathForDesktop(desktopName, fileName);
                    accept();
                }
            });

        mainLayout->addWidget(group1);
        mainLayout->addWidget(group2);
    }
};


class DynamicComboBox : public QComboBox 
{
public:
    explicit DynamicComboBox(QWidget *parent = nullptr) : QComboBox(parent) {}

public:
    void updateItems()
    {
        QSet<QString> currentItems;
        for (int i = 0; i < this->count(); i++) 
        {
            currentItems << this->itemText(i);
        }

        QSet<QString> updatedItems;
        QStringList updatedItemsList;
        auto desktops = VirtualDesktop::instance()->getDesktops();
        for(auto& desktop : desktops)
        {
            auto desktopName = VirtualDesktop::instance()->getDesktopName(desktop);
            updatedItems << desktopName;
            updatedItemsList << desktopName;
        }

        if (updatedItems != currentItems)
        {
            this->clear();
            for (const QString& desktopName : updatedItemsList)
            {
                this->addItem(desktopName);
            }
        }
    }

protected:
    void showPopup() override 
    {
        updateItems();
        QComboBox::showPopup();
    }
};

TrayIconManager::TrayIconManager(QWidget* parent) : QWidget(parent)
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto enableCustomTrayIcon = new QCheckBox("Custom tray icon per desktop");
    enableCustomTrayIcon->setChecked(Settings::instance()->customTrayIconEnabled());

    auto desktopSelector = new DynamicComboBox;
    desktopSelector->updateItems();
    desktopSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    auto desktopIcon = new QToolButton();
    desktopIcon->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    desktopIcon->setText("Configure icon");
    desktopIcon->setIcon(Settings::instance()->customTrayIconForDesktop(desktopSelector->currentText()));

    mainLayout->addWidget(enableCustomTrayIcon);
    mainLayout->addWidget(desktopSelector);
    mainLayout->addWidget(desktopIcon);
    mainLayout->addStretch();

    auto checkDesktopTimer = new QTimer(this);
    QObject::connect(checkDesktopTimer, &QTimer::timeout, this, &TrayIconManager::checkDesktopChange);
    checkDesktopTimer->setSingleShot(false);
    checkDesktopTimer->setInterval(200);

    QObject::connect(desktopIcon, &QToolButton::clicked, this, [this, desktopSelector, desktopIcon]()
        {
            auto dialog = new IconConfigureDialog(desktopSelector->currentText(), this);
            if (dialog->exec() == QDialog::Accepted)
            {
                emit updateTrayIcon(Settings::instance()->customTrayIconForDesktop(_currentDesktopName));
                desktopIcon->setIcon(Settings::instance()->customTrayIconForDesktop(desktopSelector->currentText()));
            }
        });

    QObject::connect(desktopSelector, &QComboBox::currentTextChanged, this, [this, desktopIcon](const QString& desktopName)
        {
            desktopIcon->setIcon(Settings::instance()->customTrayIconForDesktop(desktopName));
        });

    QObject::connect(enableCustomTrayIcon, &QCheckBox::toggled, this, [this, desktopSelector, checkDesktopTimer, desktopIcon](bool checked) 
        {
        Settings::instance()->setCustomTrayIconEnabled(checked);
        if (checked)
        {
            desktopSelector->setVisible(true);
            desktopIcon->setVisible(true);
            checkDesktopTimer->start();
            emit updateTrayIcon(Settings::instance()->customTrayIconForDesktop(_currentDesktopName));
        }
        else
        {
            desktopSelector->setVisible(false);
            desktopIcon->setVisible(false);
            checkDesktopTimer->stop();
            emit updateTrayIcon(QIcon(":/resources/AppIcon.ico"));
        }
        });

    _currentDesktopName = VirtualDesktop::instance()->getDesktopName(VirtualDesktop::instance()->currentDesktop());

    if (Settings::instance()->customTrayIconEnabled())
    {
        checkDesktopTimer->start();
    }
    else
    {
        desktopSelector->setVisible(false);
        desktopIcon->setVisible(false);
    }

    QTimer::singleShot(0, [this](){ onEventLoopStarted();});
}

TrayIconManager::~TrayIconManager()
{
}

void TrayIconManager::onEventLoopStarted()
{
    if (Settings::instance()->customTrayIconEnabled())
    {
        emit updateTrayIcon(Settings::instance()->customTrayIconForDesktop(_currentDesktopName));
    }
}

void TrayIconManager::checkDesktopChange()
{
    LASTINPUTINFO lastInput;
    lastInput.cbSize = sizeof(LASTINPUTINFO);
    lastInput.dwTime = 0;
    if (GetLastInputInfo(&lastInput) != 0)
    {
        if (GetTickCount64() - lastInput.dwTime > 5000)
        {
            return;
        }
    }
    else
    {
        qWarning() << "GetLastInputInfo failed!";
    }

    auto newDesktopName = VirtualDesktop::instance()->getDesktopName(VirtualDesktop::instance()->currentDesktop());
    if (newDesktopName != _currentDesktopName)
    {
        _currentDesktopName = newDesktopName;
        emit updateTrayIcon(Settings::instance()->customTrayIconForDesktop(newDesktopName));
    }
}

}