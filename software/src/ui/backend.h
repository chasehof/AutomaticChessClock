#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QTimer>

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int seconds READ seconds NOTIFY secondsChanged)

public:
    explicit Backend(QObject *parent = nullptr);

    int seconds() const;

public slots:
    void startTimer();
    void stopTimer();

signals:
    void secondsChanged();

private slots:
    void updateTimer();

private:
    QTimer m_timer;
    int m_seconds;
};

#endif // BACKEND_H