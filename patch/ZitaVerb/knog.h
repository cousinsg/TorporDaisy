#include <string>

#ifndef __KNOG_H
#define __KNOG_H

class Knog
{
public:

    Knog (void);
    ~Knog (void);

    void Init (float min, float max, float init, bool log);
    void Increment(int inc);
    float Value(void);
    float Raw(void);

private:

    float _min;
    float _max;
    float _val;
    bool _log;

};

#endif