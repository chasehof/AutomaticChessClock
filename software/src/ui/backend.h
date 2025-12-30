#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QTimer>

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int timeRemaining  READ timeRemaining NOTIFY timeChanged)
    Q_PROPERTY(QString formattedTime READ formattedTime NOTIFY timeChanged)


public:
    explicit Backend(QObject *parent = nullptr);
    int timeRemaining() const;
    QString formattedTime() const;

public slots:
    void setMaxTime(int seconds);
    void startTimer();
    void stopTimer();

signals:
    void timeChanged();
    void timerFinished();

private slots:
    void updateTimer();

private:
    QTimer m_timer;
    int m_timeRemaining;
};

#endif // BACKEND_H