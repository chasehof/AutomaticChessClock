#ifndef TIMEWINDOW_H
#define TIMEWINDOW_H

#include <QWidget>

class QLabel;
class QPushButton;
class QTimer;

class TimeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TimeWindow(QWidget *parent = nullptr);

private slots:
    void updateTime();
    void onButton1Clicked();
    void onButton2Clicked();

private:
    QLabel *timeLabel;
    QPushButton *button1;
    QPushButton *button2;
    QTimer *timer;
};

#endif // TIMEWINDOW_H