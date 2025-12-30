#include "backend.h"
#include <QString>

Backend::Backend(QObject *parent)
    : QObject(parent), m_timeRemaining(0)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &Backend::updateTimer);
}

QString Backend::formattedTime() const
{
    int minutes = m_timeRemaining / 60;
    int seconds = m_timeRemaining % 60;

    return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
}
int Backend::timeRemaining() const
{
    return m_timeRemaining;
}

void Backend::setMaxTime(int seconds)
{
    m_timeRemaining = seconds;
    emit timeChanged();
}

void Backend::startTimer()
{
    if (m_timeRemaining > 0 && !m_timer.isActive())
        m_timer.start();
}

void Backend::stopTimer()
{
    m_timer.stop();
}

void Backend::updateTimer()
{
    if (m_timeRemaining > 0) {
        m_timeRemaining--;
        emit timeChanged();
    }

    if (m_timeRemaining == 0) {
        m_timer.stop();
        emit timerFinished();
    }
}