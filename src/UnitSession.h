// UnitSession.h
#pragma once
#include <QObject>
#include <QPair>
#include <QList>

#include "UserData.h"
#include "QuestionPool.h"
#include "SpeedTracker.h"

class ZmmbTable;

class UnitSession : public QObject
{
    Q_OBJECT
public:
    explicit UnitSession(QObject *parent = nullptr);

    // 开始单元
    void start(ZbUnit *unit, ZmmbTable *zmmb, int trainMax);

    // 当前题目
    const ZbItem *currentItem() const;

    // 提交输入
    // 返回：是否正确
    bool submit(const QString &input);

    // 是否一轮结束
    bool isRoundEnd() const;

    // 是否所有轮结束
    bool isAllRoundsEnd() const;

    // 进入下一轮
    void nextRound();

    // 取下一题（返回题目索引）
    int next();

    // 进入测试模式
    void enterTestMode();

    // 当前状态
    int currentRound() const { return m_currentRound; }
    int totalRounds() const { return m_trainMax; }
    int currentIndex() const { return m_pool.currentIndex(); }
    int roundCount() const { return m_pool.currentRoundSize(); }
    int totalItems() const { return m_unit ? m_unit->items.size() : 0; }

    SpeedTracker &speed() { return m_speed; }

    // 测试模式
    bool isTestMode() const { return m_testMode; }
    void setTestItemsOverride(int count) { m_testItemsOverride = count; }
    void emitQuestionChanged() { emit questionChanged(); }
    QList<QPair<QString, QString>> getTestItems() const;
    // 上报某题答对/答错，更新 speedTable
    //   questionIndex 是题目在 m_unit->items 里的索引
    //   correct: true=答对, false=答错
    void reportTestAnswer(int questionIndex, bool correct);
    const QList<int> &poolWeightedIndexArray() const {
        return m_pool.weightedIndexArray();
    }
    struct TestItemEntry {
        QString text;
        QString code;
        int originalIndex;
    };
    QList<TestItemEntry> getTestItemsWithIndex() const;

signals:
    void questionChanged();
    void roundFinished();
    void allFinished();

private:
    QStringList getCorrectCodes(const ZbItem &item) const;
    void updateSpeedTable(int questionIndex, bool correct, qint64 elapsedMs);

    ZbUnit *m_unit = nullptr;
    ZmmbTable *m_zmmb = nullptr;
    QuestionPool m_pool;
    SpeedTracker m_speed;

    int m_trainMax = 3;
    int m_currentRound = 0;
    qint64 m_lastKeyTime = 0;

    bool m_testMode = false;
    int m_testItemsOverride = -1;
    int calcTailTestItems(int count) const;
};