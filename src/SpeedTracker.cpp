#include "SpeedTracker.h"
#include <QTime>
#include <QtMath>
#include <algorithm>

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

    // 跨会话进度：默认保留（等级只升不降）；换单元时 keepProgress=false 清零
    if (!keepProgress) {
        m_xp = 0;
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

// 升级所需 XP（等级编号从 1 开始）：A * level^1.5
long long SpeedTracker::xpNeededForLevel(int level)
{
    if (level < 1) level = 1;
    const double a = 100.0;   // 曲线系数
    return static_cast<long long>(a * qPow(static_cast<double>(level), 1.5));
}

int SpeedTracker::grade() const
{
    // 等级 = 累计 XP 达标的里程碑：只升不降
    int level = 0;
    long long acc = 0;
    while (true) {
        const long long need = xpNeededForLevel(level + 1);
        if (need <= 0) break;
        if (m_xp < acc + need) break;
        acc += need;
        ++level;
    }
    return level;
}

long long SpeedTracker::xpForNextLevel() const
{
    return xpNeededForLevel(grade() + 1);
}

long long SpeedTracker::xpAtThisLevel() const
{
    int level = 0;
    long long acc = 0;
    while (true) {
        const long long need = xpNeededForLevel(level + 1);
        if (need <= 0) break;
        if (m_xp < acc + need) break;
        acc += need;
        ++level;
    }
    return m_xp - acc;   // 当前等级内已获得的 XP
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

    // 5. 计算速度并结算经验
    updateSpeed();

    // 6. 结算本题 XP（答对才有；速度/连对作为乘数，而非等级本身）
    if (correct) {
        const double refNow = static_cast<double>(recentSpeed());
        double speedFactor = refNow / kRefSpeed;
        speedFactor = std::clamp(speedFactor, kSpeedFactorMin, kSpeedFactorMax);

        const int streakBonused = std::min(m_streak, kStreakCap);
        const double streakBonus = 1.0 + streakBonused * kStreakStep;

        const long long gained = static_cast<long long>(
            kBaseXp * gain * speedFactor * streakBonus);
        m_xp += gained;
    }
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
