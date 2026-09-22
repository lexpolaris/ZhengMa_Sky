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

    // 计时模式：
    //   true  = 有效计时（训练）：长时间不碰键盘时该段不计时
    //   false = 秒表计时（测试）：无条件累计
    void setEffectiveTiming(bool effective) { m_effectiveTiming = effective; }

    // 记录一次答题
    // correct: 是否正确
    // itemCount: 本次答对的字/词个数（通常为 1）
    // keyStrokes: 本次输入的击键次数（编码字符数，通常 >= 1）
    // elapsedMs: 反应时间（毫秒）
    void recordKey(bool correct, int itemCount, qint64 elapsedMs, int keyStrokes = 1);

    // 当前速度（字/分钟）
    int currentSpeed() const { return m_currentSpeed; }

    // 最高速度
    int bestSpeed() const { return m_bestSpeed; }

    // 等级
    int grade() const { return m_currentSpeed / 10; }

    // 击键速度（键/秒），用于曲线：数值较小，绘制时 ×10
    double hitSpeed() const { return m_hitSpeed; }

    // 测试成绩 = 平均速度 - 错误数（可为负，最高等于平均速度）
    int score() const { return m_currentSpeed - m_wrongCount; }

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

    // 有效计时的空闲阈值（ms）：训练时两次输入间隔超过此值，视为离开，该段不计时
    static constexpr qint64 kIdleThresholdMs = 5000;

private:
    void updateSpeed();

    int m_currentSpeed = 0;
    int m_bestSpeed = 0;
    double m_hitSpeed = 0.0;         // 击键速度（键/秒）
    bool m_effectiveTiming = true;   // true=训练(有效计时), false=测试(秒表计时)
    qint64 m_totalMs = 0;       // 总训练时间
    qint64 m_unitMs = 0;        // 单元训练时间
    qint64 m_sessionMs = 0;     // 本次运行时间
    int m_rightCount = 0;
    int m_wrongCount = 0;
    int m_charCount = 0;
    int m_keyStrokes = 0;       // 累计击键次数

    // 20 题窗口
    int m_windowKeys = 0;
    qint64 m_windowMs = 0;
    qint64 m_minWindowMs = 100000;
};