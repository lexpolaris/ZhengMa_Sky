// SpeedTracker.cpp
#include "SpeedTracker.h"
#include <QTime>

SpeedTracker::SpeedTracker() {}

void SpeedTracker::startSession()
{
    m_currentSpeed = 0;
    m_bestSpeed = 0;
    m_hitSpeed = 0.0;
    m_totalMs = 0;
    m_unitMs = 0;
    m_sessionMs = 0;
    m_rightCount = 0;
    m_wrongCount = 0;
    m_charCount = 0;
    m_keyStrokes = 0;
    m_windowKeys = 0;
    m_windowMs = 0;
    m_minWindowMs = 100000;
}

void SpeedTracker::resetSession()
{
    // 仅重置「本次运行」相关的累计，不影响单元进度
    m_sessionMs = 0;
    m_windowKeys = 0;
    m_windowMs = 0;
    m_minWindowMs = 100000;
}

void SpeedTracker::recordKey(bool correct, int itemCount, qint64 elapsedMs, int keyStrokes)
{
    if (elapsedMs < 0) elapsedMs = 0;
    if (keyStrokes < 1) keyStrokes = 1;

    // 有效计时（训练）：两次输入间隔过长视为离开，该段不计时
    qint64 countedMs = elapsedMs;
    if (m_effectiveTiming && countedMs > kIdleThresholdMs)
        countedMs = kIdleThresholdMs;

    // 1. 累计时间（秒表计时/有效计时共用这里累加）
    m_totalMs   += countedMs;
    m_unitMs    += countedMs;
    m_sessionMs += countedMs;

    // 2. 字词数统计：按字/词个数累加（通常为 1）
    if (correct) {
        m_charCount += (itemCount > 0 ? itemCount : 1);
        ++m_rightCount;
    } else {
        ++m_wrongCount;
    }

    // 3. 击键数累计
    m_keyStrokes += keyStrokes;

    // 4. 20 题窗口统计
    ++m_windowKeys;
    m_windowMs += countedMs;

    if (m_windowKeys >= 20) {
        if (m_windowMs < m_minWindowMs)
            m_minWindowMs = m_windowMs;
        m_windowMs = 0;
        m_windowKeys = 0;
    }

    // 5. 计算速度
    updateSpeed();
}

void SpeedTracker::updateSpeed()
{
    if (m_totalMs <= 0) {
        m_currentSpeed = 0;
        m_hitSpeed = 0.0;
        return;
    }

    // 平均速度 = 60000 * 字词数 / 总时间(ms)
    m_currentSpeed = static_cast<int>(60000LL * m_charCount / m_totalMs);

    // 击键速度 = 击键次数 / 时间(秒)
    m_hitSpeed = 1000.0 * m_keyStrokes / m_totalMs;

    if (m_currentSpeed > m_bestSpeed)
        m_bestSpeed = m_currentSpeed;
}

int SpeedTracker::accuracy() const
{
    const int total = m_rightCount + m_wrongCount;
    if (total <= 0) return 0;
    return 100 * m_rightCount / total;
}

QString SpeedTracker::formatDuration(qint64 ms)
{
    if (ms < 0) ms = 0;
    const qint64 totalSec = ms / 1000;
    const int h = static_cast<int>(totalSec / 3600);
    const int m = static_cast<int>((totalSec % 3600) / 60);
    const int s = static_cast<int>(totalSec % 60);
    return QString("%1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
}