#pragma once

#define VERSION 0.1

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