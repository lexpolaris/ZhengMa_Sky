// SpeedTracker.h
#pragma once
#include <QString>
#include <QDateTime>

class SpeedTracker
{
public:
    SpeedTracker();

    // 开始一次训练（重置）
    void startSession();

    // 记录一次击键
    // correct: 是否正确
    // targetLength: 目标编码长度
    // elapsedMs: 反应时间（毫秒）
    void recordKey(bool correct, int targetLength, qint64 elapsedMs);

    // 当前速度（字/分钟）
    int currentSpeed() const { return m_currentSpeed; }

    // 最高速度
    int bestSpeed() const { return m_bestSpeed; }

    // 等级
    int grade() const { return m_currentSpeed / 10; }

    // 总时间（毫秒）
    qint64 totalMs() const { return m_totalMs; }

    // 单元时间
    qint64 unitMs() const { return m_unitMs; }

    // 本次运行时间
    qint64 sessionMs() const { return m_sessionMs; }

    // 正确/错误数
    int rightCount() const { return m_rightCount; }
    int wrongCount() const { return m_wrongCount; }

    // 正确率（0-100）
    int accuracy() const;

    // 字数
    int charCount() const { return m_charCount; }

    // 格式化时间
    static QString formatDuration(qint64 ms);

    // 重置本次运行
    void resetSession();

private:
    void updateSpeed();

    int m_currentSpeed = 0;
    int m_bestSpeed = 0;
    qint64 m_totalMs = 0;       // 总训练时间
    qint64 m_unitMs = 0;        // 单元训练时间
    qint64 m_sessionMs = 0;     // 本次运行时间
    int m_rightCount = 0;
    int m_wrongCount = 0;
    int m_charCount = 0;

    // 20 题窗口
    int m_windowKeys = 0;
    qint64 m_windowMs = 0;
    qint64 m_minWindowMs = 100000;
};