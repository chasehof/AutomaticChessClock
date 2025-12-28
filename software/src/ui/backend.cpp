#include "backend.h"

Backend::Backend(QObject *parent)
    : QObject(parent), m_seconds(0)
{
    connect(&m_timer, &QTimer::timeout, this, &Backend::updateTimer);
    m_timer.setInterval(1000); // 1 second
}

int Backend::seconds() const
{
    return m_seconds;
}

void Backend::startTimer()
{
    if (!m_timer.isActive())
        m_timer.start();
}

void Backend::stopTimer()
{
    m_timer.stop();
}

void Backend::updateTimer()
{
    m_seconds++;
    emit secondsChanged();
}