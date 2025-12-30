#include "backend.h"
#include <QString>

Backend::Backend(QObject *parent)
    : QObject(parent), m_timeA(0), m_timeB(0)
{
    m_timerA.setInterval(1000);
    m_timerB.setInterval(1000);
    
    connect(&m_timerA, &QTimer::timeout, this, &Backend::updateTimerA);
    connect(&m_timerB, &QTimer::timeout, this, &Backend::updateTimerB);
}

void Backend::setInitialTime(int minutes, int seconds)
{
    int total = (minutes * 60) + seconds;
    m_timeA = total;
    m_timeB = total;
    emit timeChanged();
}

QString Backend::timerA() const
{
    return formatTime(m_timeA);
}

QString Backend::timerB() const
{
    return formatTime(m_timeB);
}

// LEFT BUTTON:
// Stop A ? Start B
void Backend::pressLeft()
{
    m_timerA.stop();

    if (m_timeB > 0 && !m_timerB.isActive())
        m_timerB.start();
}

// RIGHT BUTTON:
// Stop B ? Start A
void Backend::pressRight()
{
    m_timerB.stop();

    if (m_timeA > 0 && !m_timerA.isActive())
        m_timerA.start();
}

void Backend::updateTimerA()
{
    if (m_timeA > 0) {
        --m_timeA;
        emit timeChanged();
    } else {
        m_timerA.stop();
    }
}

void Backend::updateTimerB()
{
    if (m_timeB > 0) {
        --m_timeB;
        emit timeChanged();
    } else {
        m_timerB.stop();
    }
}

QString Backend::formatTime(int totalSeconds) const
{
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
}