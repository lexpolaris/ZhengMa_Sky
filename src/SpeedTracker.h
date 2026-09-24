// SpeedTracker.h
#pragma once
#include <QString>
#include <QDateTime>

class SpeedTracker
{
public:
    SpeedTracker();

    // 开始一次训练（重置本轮累计，但保留跨会话的 积分/等级/连对）
    //   keepProgress = true  时保留 m_score/m_streak（积分只增不减）
    void startSession(bool keepProgress = true);

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

    // 当前速度（字/分钟）—— 累计平均速度
    int currentSpeed() const { return m_currentSpeed; }

    // 最近速度（字/分钟）—— 基于最近 20 题窗口，反应手感，较稳定
    int recentSpeed() const;

    // 最高速度
    int bestSpeed() const { return m_bestSpeed; }

    // 等级 = 累计积分 / 10 + 1（对应原版 grade = score/10 + 1，只升不降）
    int grade() const;

    // 积分（对应原版 [self+62070]），只增不减
    int scoreTotal() const { return m_score; }

    // 累计积分（兼容旧接口名，语义等同 scoreTotal）
    long long xp() const { return m_score; }
    long long xpForNextLevel() const;
    long long xpAtThisLevel() const;

    // 连对次数
    int streak() const { return m_streak; }

    // 累加积分（每题答对时调用；points 由 UnitSession 按 LibNo 决定）
    void addScore(int points) { if (points > 0) m_score += points; }

    // 跨会话恢复：直接设置 积分 / 连对（启动或换单元时调用）
    void setXp(long long score) { m_score = score < 0 ? 0 : static_cast<int>(score); }
    void setStreak(int streak) { m_streak = streak < 0 ? 0 : streak; }

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

    // 重置本次运行（换单元时调用，回到无进度状态）
    void resetSession();

    // 有效计时的空闲阈值（ms）：训练时两次输入间隔超过此值，视为离开，该段不计时
    static constexpr qint64 kIdleThresholdMs = 5000;

    // --- 等级 / 平衡参数 ---
    static constexpr int kRecentWindow = 20;        // 最近速度窗口题数
    static constexpr int kScorePerLevel = 10;       // 每级所需积分（原版 62064=10）
    // 升级所需积分（等级编号从 1 开始）：固定 kScorePerLevel
    static long long xpNeededForLevel(int level);

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

    // 跨会话进度（只增不减）
    int m_score = 0;            // 累计积分（对应原版 [self+62070]）
    int m_streak = 0;           // 当前连对次数

    // 最近 N 题窗口（用于 recentSpeed）
    int m_winKeys = 0;          // 窗口内答对数
    qint64 m_winMs = 0;         // 窗口内累计时间
    int m_recentSpeed = 0;      // 最近窗口速度缓存
};