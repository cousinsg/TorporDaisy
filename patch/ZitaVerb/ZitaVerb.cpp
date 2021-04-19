#include "daisySP.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

static DaisyPatch patch;

static void AudioCallback(float **in, float **out, size_t size)
{
}

int main(void)
{
    patch.StartAdc();
    patch.StartAudio(AudioCallback);
}