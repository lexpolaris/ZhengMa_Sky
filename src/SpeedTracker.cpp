// SpeedTracker.cpp
#include "SpeedTracker.h"
#include <QTime>

SpeedTracker::SpeedTracker() {}

void SpeedTracker::startSession()
{
    m_currentSpeed = 0;
    m_bestSpeed = 0;
    m_totalMs = 0;
    m_unitMs = 0;
    m_rightCount = 0;
    m_wrongCount = 0;
    m_charCount = 0;
    m_windowKeys = 0;
    m_windowMs = 0;
    m_minWindowMs = 100000;
}

void SpeedTracker::recordKey(bool correct, int targetLength, qint64 elapsedMs)
{
    // 1. 累计时间
    m_totalMs  += elapsedMs;
    m_unitMs   += elapsedMs;
    m_sessionMs += elapsedMs;

    // 2. 字数统计
    if (correct) {
        m_charCount += (targetLength > 0 ? targetLength : 1);
        ++m_rightCount;
    } else {
        ++m_wrongCount;
    }

    // 3. 20 题窗口统计
    ++m_windowKeys;
    m_windowMs += elapsedMs;

    if (m_windowKeys >= 20) {
        if (m_windowMs < m_minWindowMs)
            m_minWindowMs = m_windowMs;
        m_windowMs = 0;
        m_windowKeys = 0;
    }

    // 4. 计算速度
    updateSpeed();
}

void SpeedTracker::updateSpeed()
{
    if (m_totalMs <= 0) {
        m_currentSpeed = 0;
        return;
    }

    // 速度 = 60000 * 字数 / 总时间(ms)
    m_currentSpeed = static_cast<int>(60000LL * m_charCount / m_totalMs);

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