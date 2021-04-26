#include <math.h>
#include "knog.h"

Knog::Knog (void)
{
}

Knog::~Knog (void)
{
}

void Knog::Init(float min, float max, float init, bool log)
{
    if (init < min || init > max)
    {
        init = min;
    }

    if (log)
    {
        _min = logf(min < 0.0000001 ? 0.0000001 : min);
        _max = logf(max);
        init = logf(init);
    }
    else 
    {
        _min = min;
        _max = max;
    }

    _val = (init - _min) / (_max - _min);
    _log = log;
}

void Knog::Increment(int inc)
{
    _val += 0.01 * inc;

    if (_val < 0)
    {
        _val = 0;
    }
    else if (_val > 1)
    {
        _val = 1;
    }
}

float Knog::Value()
{
    float val = (_val * (_max - _min)) + _min;
    return _log ? expf(val) : val;
}

float Knog::Raw()
{
    return _val;
}