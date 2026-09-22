// TestHistory.h
#pragma once
#include <QString>
#include <QList>
#include <QDateTime>

struct TestRecord {
    int libNo = 0;
    QString libName;
    int speed = 0;          // 平均速度（字词/分钟）
    int bestSpeed = 0;      // 最高速度
    double hitSpeed = 0.0;  // 击键速度（键/秒），绘制曲线时 ×10
    int accuracy = 0;       // 正确率（%）
    int wrongCount = 0;     // 错误数
    qint64 totalMs = 0;     // 总时间
    QDateTime dateTime;     // 测试时间

    // 测试成绩 = 平均速度 - 错误数（可为负，最高等于平均速度）
    int score() const { return speed - wrongCount; }
};

class TestHistory
{
public:
    TestHistory();

    bool load(const QString &filePath);
    bool save(const QString &filePath) const;

    void addRecord(const TestRecord &rec);

    const QList<TestRecord>& records() const { return m_records; }
    QList<TestRecord> recordsByUnit(int libNo) const;

    void clear() { m_records.clear(); }

    static constexpr int kMaxRecords = 10;   // 原程序保留最近 10 次

private:
    QList<TestRecord> m_records;
};