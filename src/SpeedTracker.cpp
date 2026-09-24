#include "SpeedTracker.h"
#include <QTime>

SpeedTracker::SpeedTracker() {}

void SpeedTracker::startSession(bool keepProgress)
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
    m_winKeys = 0;
    m_winMs = 0;
    m_recentSpeed = 0;

    // 跨会话进度：默认保留（积分只增不减）；换单元时 keepProgress=false 清零
    if (!keepProgress) {
        m_score = 0;
        m_streak = 0;
    }
}

void SpeedTracker::resetSession()
{
    // 仅重置「本次运行」相关的累计，不影响单元进度与跨会话 XP
    m_sessionMs = 0;
    m_winKeys = 0;
    m_winMs = 0;
    m_recentSpeed = 0;
}

// 升级所需积分（等级编号从 1 开始）：每级固定 kScorePerLevel
long long SpeedTracker::xpNeededForLevel(int level)
{
    if (level < 1) level = 1;
    return static_cast<long long>(kScorePerLevel);
}

int SpeedTracker::grade() const
{
    // 等级 = 累计积分 / 10 + 1（对应原版 grade = score/10 + 1）
    return m_score / kScorePerLevel + 1;
}

long long SpeedTracker::xpForNextLevel() const
{
    // 升到下一级的累计积分门槛（按当前等级线性累加）
    return static_cast<long long>(grade()) * kScorePerLevel;
}

long long SpeedTracker::xpAtThisLevel() const
{
    // 当前等级内已获得的积分（0..kScorePerLevel-1）
    return m_score % kScorePerLevel;
}

int SpeedTracker::recentSpeed() const
{
    if (m_winMs > 0 && m_winKeys > 0)
        return static_cast<int>(60000LL * m_winKeys / m_winMs);
    return m_currentSpeed;
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
    const int gain = (itemCount > 0 ? itemCount : 1);
    if (correct) {
        m_charCount += gain;
        ++m_rightCount;
        ++m_streak;
    } else {
        ++m_wrongCount;
        m_streak = 0;        // 答错打断连对
    }

    // 3. 击键数累计
    m_keyStrokes += keyStrokes;

    // 4. 最近窗口统计（用于 recentSpeed，比累计平均稳定）
    ++m_winKeys;
    m_winMs += countedMs;
    if (m_winKeys >= kRecentWindow || m_winMs >= kIdleThresholdMs * 4) {
        if (m_winMs > 0)
            m_recentSpeed = static_cast<int>(60000LL * m_winKeys / m_winMs);
        m_winKeys = 0;
        m_winMs = 0;
    }

    // 5. 计算速度
    updateSpeed();

    // 积分（[self+62070]）由 UnitSession 在答对时按 LibNo 累加，
    // 见 UnitSession::submit() → SpeedTracker::addScore()
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
