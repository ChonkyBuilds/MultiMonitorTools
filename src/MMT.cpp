#include "MMT.hpp"
#include <omp.h>

namespace MMT
{

StopWatch::StopWatch()
{
    _startTime = omp_get_wtime() * 1000.0;
}

StopWatch::~StopWatch()
{

}

double StopWatch::elapsed()
{
    return omp_get_wtime() * 1000.0 - _startTime;
}

void StopWatch::start()
{
    _startTime = omp_get_wtime() * 1000.0;
}

}

