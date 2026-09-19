// QuestionPool.h
#pragma once
#include <QList>
#include <QRandomGenerator>

class QuestionPool
{
public:
    QuestionPool();

    void init(int totalItems,
              int roundCount,
              const QList<int> &speedTable = {},
              int errorRepeat = 3);

    int next();
    int currentIndex() const { return m_currentIndex; }
    bool isRoundEnd() const;
    void reshuffle();
    void reset();
    void setSpeedTable(const QList<int> &speeds);
    QList<int> &speedTable() { return m_speedTable; }
    const QList<int> &speedTable() const { return m_speedTable; }

    // 当前题目的原始索引
    int currentQuestionIndex() const;
    
    // 加权池（供测试模式使用）
    const QList<int> &weightedIndexArray() const { return m_weightedIndex; }

    // 当前轮次的题数（以加权池为准）
    int currentRoundSize() const { return m_roundCount; }

private:
    void sortBySpeed();
    void shuffle();
    void buildWeightedPool(int errorRepeat);

    int m_totalItems = 0;
    int m_roundCount = 0;
    int m_currentIndex = -1;
    QList<int> m_speedTable;
    QList<int> m_weightedIndex;
};