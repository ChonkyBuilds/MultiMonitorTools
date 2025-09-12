#include "MMTMonitorConfigWidget.hpp"
#include "MMTMonitorManager.hpp"
#include "MMTMonitorLayoutEditor.hpp"

#include <QVBoxLayout>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QPushButton>
#include <qDebug>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QScrollBar>
#include <QLabel>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGuiApplication>
#include <QGroupBox>

namespace MMT {

MonitorConfigWidget::MonitorConfigWidget(QWidget* parent) : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);
    setLineWidth(2);

    _layoutEditor = new MonitorLayoutEditor();
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(_layoutEditor);

    QObject::connect(_layoutEditor, &MonitorLayoutEditor::screenGeometryChanged, this, &MonitorConfigWidget::onScreenGeometryChanged);
    QObject::connect(_layoutEditor, &MonitorLayoutEditor::screenSelectionChanged, this, &MonitorConfigWidget::refreshScreenParameters);

    _x = new  QDoubleSpinBox();
    _y = new QDoubleSpinBox();
    _size = new QDoubleSpinBox();
    _x->setRange(-9999,9999);
    _x->setSingleStep(0.1);
    _y->setRange(-9999,9999);
    _y->setSingleStep(0.1);
    _size->setRange(1, 9999);
    _size->setSingleStep(0.1);

    _enableVirtualDesktop = new QCheckBox(MonitorConfigWidget::tr("Enable virtual desktops"));
    QObject::connect(_enableVirtualDesktop, &QCheckBox::toggled, this, &MonitorConfigWidget::onEnableVirtualDesktopToggled);
    QObject::connect(_x, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MonitorConfigWidget::onPhysicalRectChanged);
    QObject::connect(_y, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MonitorConfigWidget::onPhysicalRectChanged);
    QObject::connect(_size, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MonitorConfigWidget::onPhysicalRectChanged);

    auto displayParametersBox = new QGroupBox;
    auto displayParametersBoxLayout = new QHBoxLayout(displayParametersBox);
    displayParametersBox->setTitle(MonitorConfigWidget::tr("Display physical coordinates"));
    displayParametersBoxLayout->addWidget(new QLabel(MonitorConfigWidget::tr("X(mm)")));
    displayParametersBoxLayout->addWidget(_x);
    displayParametersBoxLayout->addWidget(new QLabel(MonitorConfigWidget::tr("Y(mm)")));
    displayParametersBoxLayout->addWidget(_y);
    displayParametersBoxLayout->addWidget(new QLabel(MonitorConfigWidget::tr("Width(mm)")));
    displayParametersBoxLayout->addWidget(_size);
    displayParametersBoxLayout->addStretch();
    mainLayout->addWidget(displayParametersBox);

    auto vdSettingsBox = new QGroupBox;
    vdSettingsBox->setTitle(MonitorConfigWidget::tr("Virtual desktop settings"));
    auto vdSettingsBoxLayout = new QHBoxLayout(vdSettingsBox);
    vdSettingsBoxLayout->addWidget(_enableVirtualDesktop);
    mainLayout->addWidget(vdSettingsBox);

    //auto cursorSettings = new QGroupBox;
    //cursorSettings->setTitle(MonitorConfigWidget::tr("Cursor settings"));
    //auto cursorSettingsLayout = new QHBoxLayout(cursorSettings);
    //cursorSettingsLayout->addWidget(new QLabel(MonitorConfigWidget::tr("Cursor speed: ")));
    //cursorSettingsLayout->addWidget(new QSpinBox());
    //cursorSettingsLayout->addStretch();
    //mainLayout->addWidget(cursorSettings);    

    setupScreens();
}

MonitorConfigWidget::~MonitorConfigWidget()
{
}

void MonitorConfigWidget::setupScreens()
{
    _layoutEditor->clearScreens();
    auto screens = MonitorManager::instance()->screens();
    for (auto& screen : screens)
    {
        _layoutEditor->addScreen(screen);
    }
    _layoutEditor->alignScreens();

    refreshScreenParameters(_layoutEditor->selectedScreen());
}

void MonitorConfigWidget::onScreenGeometryChanged(const Screen* screen, const QRectF& geometry)
{
    for (auto s : MonitorManager::instance()->screens())
    {
        if (s == screen)
        {
            s->setPhysicalRect(geometry);
        }
    }

    if (screen == _layoutEditor->selectedScreen())
    {
        _x->blockSignals(true);
        _y->blockSignals(true);
        _size->blockSignals(true);

        _x->setValue(geometry.x());
        _y->setValue(geometry.y());
        _size->setValue(geometry.width());

        _x->blockSignals(false);
        _y->blockSignals(false);
        _size->blockSignals(false);
    }

    emit screenGeometryChanged();
}

void MonitorConfigWidget::refreshScreenParameters(const Screen* screen)
{
    _x->blockSignals(true);
    _y->blockSignals(true);
    _size->blockSignals(true);
    _enableVirtualDesktop->blockSignals(true);

    auto physicalRect = screen->physicalRect();

    _x->setValue(physicalRect.x());
    _y->setValue(physicalRect.y());
    _size->setValue(physicalRect.width());
    _enableVirtualDesktop->setChecked(screen->virtualDesktopEnabled());

    _x->blockSignals(false);
    _y->blockSignals(false);
    _size->blockSignals(false);
    _enableVirtualDesktop->blockSignals(false);
}

void MonitorConfigWidget::onEnableVirtualDesktopToggled(bool flag)
{
    for (auto screen : MonitorManager::instance()->screens())
    {
        if (screen == _layoutEditor->selectedScreen())
        {
            screen->setVirtualDesktopEnabled(flag);
        }
    }
}

void MonitorConfigWidget::resizeEvent(QResizeEvent* event)
{
    _layoutEditor->zoomToFit();
    QFrame::resizeEvent(event);
}

void MonitorConfigWidget::onPhysicalRectChanged()
{
    for (auto screen : MonitorManager::instance()->screens())
    {
        if (screen == _layoutEditor->selectedScreen())
        {
            QRectF geometry(_x->value(), _y->value(),_size->value(), _size->value() / screen->physicalCoordinateRect().width() * screen->physicalCoordinateRect().height());
            _layoutEditor->tryModifyScreenGeometry(screen, geometry);
            screen->setPhysicalRect(geometry);        
        }
    }
}

}

