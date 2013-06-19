#pragma once

struct Signal;

enum SignalType { NONE = -1, LEVEL, PULSE, CLOCK };

struct TimingParams {
    SignalType type;
    Signal* signal;
    int address : 4;
    const char* const label;
};
