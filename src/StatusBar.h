// StatusBar.h
#pragma once
#include <QWidget>
#include <QLabel>

class StatusBar : public QWidget
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = nullptr);

    void setMode(const QString &mode);      // "游戏闯关"
    void setUnitName(const QString &name);  // "第一主根"
    void setClock(const QString &time);     // "12:34:56"

private:
    QLabel *m_labelMode;
    QLabel *m_labelUnit;
    QLabel *m_labelClock;
};
