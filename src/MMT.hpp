#pragma once
#include <memory>

#define DECLARE_IMPL(C) struct C##Impl; std::unique_ptr<C##Impl> _impl
#define INIT_IMPL(C) _impl = std::make_unique<C##Impl>()
#define IMPL(C) struct C::C##Impl
#define D_Ptr(C) C##Impl* d = _impl.get()
#define CONST_D_Ptr(C) const C##Impl* d = _impl.get()

namespace MMT
{

enum class Direction
{
    Left,
    Right
};

class StopWatch
{
public:
    StopWatch();
    ~StopWatch();

    void start();
    double elapsed();

private:
    double _startTime;
};


}