#pragma once

#include <QObject>

namespace MMT
{

class CursorAdjust : public QObject
{
    Q_OBJECT
public:
    static CursorAdjust* instance();

public slots:
    virtual void initiate() = 0;
    virtual void exit() = 0;

protected:
    CursorAdjust() = default;
    ~CursorAdjust() = default;
    CursorAdjust(CursorAdjust const&) = delete;
    void operator=(CursorAdjust const&) = delete;
};

}