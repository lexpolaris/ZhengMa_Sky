// InfoPanel.h
#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

class InfoPanel : public QWidget
{
    Q_OBJECT
public:
    explicit InfoPanel(QWidget *parent = nullptr);

    // 状态更新接口
    void setUnitName(const QString &name);
    void setGrade(int grade);
    void setTotalTime(const QString &time);
    void setUnitTime(const QString &time);
    void setSessionTime(const QString &time);
    void setCharCount(int count);
    void setAccuracy(int percent);
    void setSpeed(int current, int best);
    void setLookupResult(const QString &text);
    // 训练进度：
    //   roundLeft  本轮剩余题数（倒计数）
    //   roundTotal 本轮总题数
    //   roundsLeft 剩余轮数（倒计数）
    //   roundsTotal 总轮数
    //   roundsDone 已完成轮数
    void setProgress(int roundLeft, int roundTotal,
                     int roundsLeft, int roundsTotal,
                     int roundsDone);

signals:
    void switchModeClicked();
    void selectLibClicked();
    void setupClicked();
    void statusClicked();
    void helpClicked();
    void quitClicked();
    void lookupChanged(const QString &text);

private:
    QLabel *m_labelUnitName;
    QLabel *m_labelGrade;
    QLabel *m_labelTotalTime;
    QLabel *m_labelUnitTime;
    QLabel *m_labelSessionTime;
    QLabel *m_labelCharCount;
    QLabel *m_labelProgress;
    QLabel *m_labelAccuracy;
    QLabel *m_labelSpeed;
    QLineEdit *m_editLookup;
    QLabel    *m_labelLookupResult;
};