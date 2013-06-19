#pragma once

#include <vector>
#include <stdexcept>

#define TOTAL_LENGTH (1 << 14)

class QSettings;

// some sort of an abstract signal with fixed length
struct Signal {
    virtual ~Signal() {}

    virtual void loadSettings(QSettings* settings) = 0;
    virtual void saveSettings(QSettings* settings) = 0;
    virtual void form(std::vector<bool>&) const = 0;
    static void inverse(std::vector<bool>&);
    virtual void changeRatio() = 0;

    static unsigned int total_length;
    static double ratio;
};

// "0" or "1"
struct Level : public Signal {
    Level(bool invert = false) : inverted(invert) {}
    virtual ~Level() {}

    virtual void loadSettings(QSettings* settings);
    virtual void saveSettings(QSettings* settings);
    virtual void form(std::vector<bool>&) const;
    virtual void changeRatio();

    bool inverted; // logical level
};

// single pulse
struct Pulse : public Level {
    Pulse( unsigned int pulse_offset = 0, unsigned int pulse_length = 0)
        : Level(), offset(pulse_offset), length(pulse_length) {}

    Pulse( bool signal_invert, unsigned int pulse_offset = 0, unsigned int pulse_length = 0)
        : Level(signal_invert), offset(pulse_offset), length(pulse_length) {}
    virtual ~Pulse() {}

    virtual void loadSettings(QSettings* settings);
    virtual void saveSettings(QSettings* settings);

    virtual void form(std::vector<bool>&) const throw (std::overflow_error);
    virtual void form( std::vector<bool>& vec, std::vector<bool>& pulse) const throw (std::overflow_error);
    virtual void changeRatio();

    unsigned int offset;
    unsigned int length;
};

// clock, multiple pulses (repeated many times)
struct Clock : public Pulse {
    Clock( bool signal_invert, unsigned int clock_offset, unsigned int pulse_offset, unsigned int pulse_length, unsigned int clock_times)
        : Pulse( signal_invert, clock_offset), pulse( pulse_offset, pulse_length), times(clock_times) {}
    virtual ~Clock() {}

    virtual void loadSettings(QSettings* settings);
    virtual void saveSettings(QSettings* settings);

    virtual void form(std::vector<bool>&) const throw (std::overflow_error);
    virtual void changeRatio();

    Pulse pulse;
    unsigned int times;
};
