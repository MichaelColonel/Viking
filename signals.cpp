#include <functional>

#include <QSettings>

#include "signals.h"

unsigned int Signal::total_length = TOTAL_LENGTH;
double Signal::ratio = 1.0;

void
Signal::inverse(std::vector<bool>& vec)
{
    std::transform( vec.begin(), vec.end(), vec.begin(), std::logical_not<bool>());
}

void
Level::saveSettings(QSettings* settings)
{
    settings->setValue( "signal-logical-level", inverted);
}

void
Level::loadSettings(QSettings* settings)
{
    inverted = settings->value( "signal-logical-level", 0).toBool();
}

void
Level::form(std::vector<bool>& vec) const { if (inverted) inverse(vec); }

void
Level::changeRatio()
{
}

void
Pulse::saveSettings(QSettings* settings)
{
    Level::saveSettings(settings);
    settings->setValue( "pulse-offset", offset);
    settings->setValue( "pulse-length", length);
}

void
Pulse::loadSettings(QSettings* settings)
{
    Level::loadSettings(settings);
    offset = settings->value( "pulse-offset", 0).toInt();
    length = settings->value( "pulse-length", 0).toInt();
}

void
Pulse::form(std::vector<bool>& vec) const throw (std::overflow_error)
{
    Level::form(vec);
/*
    if (offset >= vec.size())
        throw std::overflow_error("Pulse offset is too big.");

    if (offset + length >= vec.size())
        throw std::overflow_error("Pulse (offset + lenght) sum is too big.");
*/
    int rel_offset = vec.size() - offset;
    int rel_offlen = vec.size() - offset - length;
    if (rel_offset >= 0 && rel_offlen >= 0)
        std::fill( vec.begin() + offset, vec.begin() + offset + length, !inverted);
    else if (rel_offset >= 0 && rel_offlen < 0)
        std::fill( vec.begin() + offset, vec.end(), !inverted);
}

void
Pulse::form( std::vector<bool>& vec, std::vector<bool>& pulse) const throw (std::overflow_error)
{
    Level::form(vec);
    int rel_offset = vec.size() - offset;
    int rel_offlen = vec.size() - offset - pulse.size();
    if (rel_offset >= 0 && rel_offlen >= 0)
        std::copy( pulse.begin(), pulse.end(), vec.begin() + offset);
    else if (rel_offset >= 0 && rel_offlen < 0)
        std::copy( pulse.begin(), pulse.end() + rel_offlen, vec.begin() + offset);
}

void
Pulse::changeRatio()
{
    offset /= Signal::ratio;
    length /= Signal::ratio;
}

void
Clock::saveSettings(QSettings* settings)
{
    Level::saveSettings(settings);
    settings->setValue( "clock-offset", offset);

    settings->beginGroup("clock-pulse");
    pulse.saveSettings(settings);
    settings->endGroup();

    settings->setValue( "clock-times", times);
}

void
Clock::loadSettings(QSettings* settings)
{
    Level::loadSettings(settings);
    offset = settings->value( "clock-offset", 0).toInt();

    settings->beginGroup("clock-pulse");
    pulse.loadSettings(settings);
    settings->endGroup();

    times = settings->value( "clock-times", 0).toInt();
}

void
Clock::form(std::vector<bool>& vec) const throw (std::overflow_error)
{
    const int& start = pulse.offset;
    const int& len = pulse.length;

    std::vector<bool> ptime(start + len);
    std::fill( ptime.begin() + start, ptime.begin() + start + len, 1);
    pulse.Level::form(ptime);


    int rel_offset = vec.size() - offset;
    int rel_offlen = vec.size() - (start + len) * times - offset;

/*    if (ptime.size() * times + offset >= vec.size())
        throw std::overflow_error("Clock pulse values are too big.");
*/
    if (rel_offset >= 0 && rel_offlen >= 0) {
        for ( unsigned int i = 0, off = offset; i < times; ++i) {
            std::copy( ptime.begin(), ptime.end(), vec.begin() + off);
            off += ptime.size();
        }
    }
    else if (rel_offset >= 0 && rel_offlen < 0) {
        std::vector<bool> clock(ptime.size() * times);
        for ( unsigned int i = 0; i < times; ++i)
            std::copy( ptime.begin(), ptime.end(), clock.begin() + i * ptime.size());

        Pulse::form( vec, clock);
    }

    Level::form(vec);
}

void
Clock::changeRatio()
{
    Pulse::changeRatio();
    pulse.Pulse::changeRatio();
}
