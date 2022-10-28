#include "daisySP.h"
#include "daisy_patch.h"
#include "reverb.h"
#include "knog.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
Reverb verb;
DcBlock blk[2];

Knog xover;
Knog rtlow;
Knog rtmid;
Knog fdamp;

Knog eq1fr;
Knog eq1gn;
Knog eq2fr;
Knog eq2gn;

enum State : uint8_t
{
    ParamSelect    = 0,
    Xover          = 1,
    RtLow          = 2,
    RtMid          = 3,
    Fdamp          = 4,
    Eq1Fr          = 5,
    Eq1Gn          = 6,
    Eq2Fr          = 7,
    Eq2Gn          = 8
};
constexpr uint8_t NUM_STATES = 9;
constexpr uint8_t NUM_PARAM  = NUM_STATES - 1;
State             currState  = State::ParamSelect;
uint8_t           currParam  = 0;

void UpdateParamState()
{
    if(patch.encoder.FallingEdge())
    {
        switch(currState)
        {
            case State::ParamSelect:
                currState = static_cast<State>(currParam + 1);
                break;
            default: currState = State::ParamSelect; break;
        }
    }

    const int32_t currInc = patch.encoder.Increment();

    if(abs(currInc) > 0)
    {
        switch(currState)
        {
            case State::ParamSelect:
            {
                int32_t newValue = currParam + currInc;
                newValue += newValue < 0 ? NUM_PARAM : 0;
                currParam = newValue % NUM_PARAM;
                break;
            }
            case State::Xover:
            {
                xover.Increment(currInc);
                verb.set_xover(xover.Value());
                break;
            }
            case State::RtLow:
            {
                rtlow.Increment(currInc);
                verb.set_rtlow(rtlow.Value());
                break;
            }
            case State::RtMid:
            {
                rtmid.Increment(currInc);
                verb.set_rtmid(rtmid.Value());
                break;
            }
            case State::Fdamp:
            {
                fdamp.Increment(currInc);
                verb.set_fdamp(fdamp.Value());
                break;
            }
            case State::Eq1Fr:
            {
                eq1fr.Increment(currInc);
                verb.set_eq1(eq1fr.Value(), eq1gn.Value());
                break;
            }
            case State::Eq1Gn:
            {
                eq1gn.Increment(currInc);
                verb.set_eq1(eq1fr.Value(), eq1gn.Value());
                break;
            }
            case State::Eq2Fr:
            {
                eq2fr.Increment(currInc);
                verb.set_eq2(eq2fr.Value(), eq2gn.Value());
                break;
            }
            case State::Eq2Gn:
            {
                eq2gn.Increment(currInc);
                verb.set_eq2(eq2fr.Value(), eq2gn.Value());
                break;
            }
        }
    }
}

void UpdateControls()
{
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();

    UpdateParamState();

    float delay = patch.GetKnobValue(patch.CTRL_3);
    verb.set_delay((delay * (0.1 - 0.02)) + 0.02);

    verb.set_opmix(patch.GetKnobValue(patch.CTRL_4));
}

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    UpdateControls();

    for (size_t i = 0; i < size; i++)
    {
        float drylevel = patch.GetKnobValue(patch.CTRL_1);
        float sendLevel = patch.GetKnobValue(patch.CTRL_2);

        float wetL, wetR;

        float dryL = in[0][i];
        float dryR = in[1][i];

        float sendL = dryL * sendLevel;
        float sendR = dryR * sendLevel;

        float *send [2] = {&sendL, &sendR};
        float *wet [2] = {&wetL, &wetR};

        verb.prepare(1);
        verb.process(1, send, wet);

        wetL = blk[0].Process(*wet[0]);
        wetR = blk[1].Process(*wet[1]);

        out[0][i] = (dryL * drylevel) + wetL;
        out[1][i] = (dryR * drylevel) + wetR;

        out[2][i] = wetL;
        out[3][i] = wetR;
    }
}

void DrawKnob(int x, int y, int r, float v, std::string label, std::string min, std::string max, bool selected, bool active)
{
    patch.display.DrawArc(x, y, r + 1, 225, -270, true);

    if (active)
    {
        patch.display.DrawCircle(x, y, 2, true);
    }

    float a = (((v * 270) + 45) * M_PI / -180);
    float x1 = static_cast<int>(x + (r * sin(a)));
    float y1 = static_cast<int>(y + (r * cos(a)));

    patch.display.DrawLine(x, y, x1, y1, true);

    char *cstrl = &label[0];
    char *cstrm = &min[0];
    char *cstrx = &max[0];


    patch.display.SetCursor(x - 15, y + 24);
    patch.display.WriteString(cstrl, Font_6x8, !selected);

    patch.display.SetCursor(x - 26, y + 16);
    patch.display.WriteString(cstrm, Font_6x8, true);

    patch.display.SetCursor(x + 9, y + 16);
    patch.display.WriteString(cstrx, Font_6x8, true);
}

void UpdateDisplay()
{
    patch.display.Fill(false);

    bool highlightState[NUM_STATES]{false};
    highlightState[static_cast<uint8_t>(currState)] = true;
    if(highlightState[static_cast<uint8_t>(State::ParamSelect)])
    {
        highlightState[1 + currParam] = true;
    }

    std::string str  = "ZitaVerb";
    char *cstr = &str[0];
    patch.display.SetCursor(0, 0);
    patch.display.WriteString(cstr, Font_6x8, true);

    switch(currParam)
    {
        case 0:
        case 1:
        {
            DrawKnob(32, 32, 15, xover.Raw(), "XOVER", " 20", "100", highlightState[static_cast<uint8_t>(State::Xover)], currState == State::Xover);
            DrawKnob(96, 32, 15, rtlow.Raw(), "RTLOW", "  1", "8", highlightState[static_cast<uint8_t>(State::RtLow)], currState == State::RtLow);
            break;
        }
        case 2:
        case 3:
        {
            DrawKnob(32, 32, 15, rtmid.Raw(), "RTMID", "  1", "8", highlightState[static_cast<uint8_t>(State::RtMid)], currState == State::RtMid);
            DrawKnob(96, 32, 15, fdamp.Raw(), "FDAMP", "1.5", "24k", highlightState[static_cast<uint8_t>(State::Fdamp)], currState == State::Fdamp);
            break;
        }
        case 4:
        case 5:
        {
            DrawKnob(32, 32, 15, eq1fr.Raw(), "EQ1FR", " 40", "2k5", highlightState[static_cast<uint8_t>(State::Eq1Fr)], currState == State::Eq1Fr);
            DrawKnob(96, 32, 15, eq1gn.Raw(), "EQ1GN", "-15", "+15", highlightState[static_cast<uint8_t>(State::Eq1Gn)], currState == State::Eq1Gn);
            break;
        }
        case 6:
        case 7:
        {
            DrawKnob(32, 32, 15, eq2fr.Raw(), "EQ2FR", "160", "10k", highlightState[static_cast<uint8_t>(State::Eq2Fr)], currState == State::Eq2Fr);
            DrawKnob(96, 32, 15, eq2gn.Raw(), "EQ2GN", "-15", "+15", highlightState[static_cast<uint8_t>(State::Eq2Gn)], currState == State::Eq2Gn);
            break;
        }
    }

    patch.display.Update();
}

int main(void)
{
    float samplerate;
    patch.Init();
    samplerate = patch.AudioSampleRate();

    xover.Init(50, 1000, 200, true);
    rtlow.Init(1, 8, 3, true);
    rtmid.Init(1, 8, 2, true);
    fdamp.Init(1.5e3, 24.0e3, 6.0e3, true);

    eq1fr.Init(40.0, 2.5e3, 160.0, true);
    eq1gn.Init(-15, 15, 0, false);
    eq2fr.Init(160.0, 10e3, 2.5e3, true);
    eq2gn.Init(-15, 15, 0, false);

    verb.init(samplerate, false);

    blk[0].Init(samplerate);
    blk[1].Init(samplerate);

    patch.StartAdc();
    patch.StartAudio(AudioCallback);

    while(1)
    {
        UpdateDisplay();
    }
}