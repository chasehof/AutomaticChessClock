#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QTimer>

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString timerA READ timerA NOTIFY timeChanged)
    Q_PROPERTY(QString timerB READ timerB NOTIFY timeChanged)


public:
    explicit Backend(QObject *parent = nullptr);
    int timeRemaining() const;
    
    QString timerA() const;
    QString timerB() const;

public slots:
    void setInitialTime(int minutes, int seconds);

    // Button actions
    void pressLeft();   // stop A, start B
    void pressRight();  // stop B, start A
signals:
    void timeChanged();
    void timerFinished();

private slots:
    void updateTimerA();
    void updateTimerB();

private:
    QTimer m_timerA;
    QTimer m_timerB;

    int m_timeA;
    int m_timeB;
    
    QString formatTime(int totalSeconds) const;
};

#endif // BACKEND_H