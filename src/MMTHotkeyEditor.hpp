#include "MMT.hpp"
#include "MMTHotkeys.hpp"

#include <QDialog>

namespace MMT
{

class HotkeyEditorItem;
class HotkeyEditor : public QDialog
{
public:
    HotkeyEditor(QWidget* parent = nullptr);
    ~HotkeyEditor();

private slots:
    void onAccepted();

private:
    void deleteItem(QWidget*);

private:
    QWidget* _listWidget = nullptr;
    friend class HotkeyEditorItem;
};

}