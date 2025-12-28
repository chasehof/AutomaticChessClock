#include "TimeWindow.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QTime>

TimeWindow::TimeWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Qt5 Time UI");
    resize(300, 150);

    timeLabel = new QLabel(this);
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setStyleSheet("font-size: 24px;");

    button1 = new QPushButton("Button 1", this);
    button2 = new QPushButton("Button 2", this);

    connect(button1, &QPushButton::clicked,
            this, &TimeWindow::onButton1Clicked);
    connect(button2, &QPushButton::clicked,
            this, &TimeWindow::onButton2Clicked);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(button1);
    buttonLayout->addWidget(button2);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(timeLabel);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this, &TimeWindow::updateTime);
    timer->start(1000);

    updateTime();
}

void TimeWindow::updateTime()
{
    timeLabel->setText(QTime::currentTime().toString("HH:mm:ss"));
}

void TimeWindow::onButton1Clicked()
{
    qDebug("Button 1 clicked");
}

void TimeWindow::onButton2Clicked()
{
    qDebug("Button 2 clicked");
}

