#pragma once

#include <QFrame>

class QRectF;
class QString;
class QResizeEvent;
class QCheckBox;
class QGroupBox;
class QDoubleSpinBox;

namespace MMT{

class Screen;
class MonitorLayoutEditor;

class MonitorConfigWidget : public QFrame
{
    Q_OBJECT
public:
    MonitorConfigWidget(QWidget* parent = 0);
    ~MonitorConfigWidget();

public:
    void setupScreens();

public slots:
    void onScreenGeometryChanged(const Screen* screen, const QRectF& geometry);
    void refreshScreenParameters(const Screen*);

signals:
    void screenGeometryChanged();

private slots:
    void onPhysicalRectChanged();
    void onEnableVirtualDesktopToggled(bool);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QCheckBox* _pinWindows = nullptr;
    QCheckBox* _enableAnimation = nullptr;
    QCheckBox* _enablePrompt = nullptr;
    QGroupBox* _groupBox = nullptr;
    QDoubleSpinBox* _x = nullptr;
    QDoubleSpinBox* _y = nullptr;
    QDoubleSpinBox* _size = nullptr;
    QCheckBox* _enableVirtualDesktop = nullptr;
    MonitorLayoutEditor* _layoutEditor = nullptr;
};

}