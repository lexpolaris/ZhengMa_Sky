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
              const QList<int> &speedTable = {});

    int next();
    int currentIndex() const { return m_currentIndex; }
    bool isRoundEnd() const;
    void reshuffle();
    void reset();

    void setSpeedTable(const QList<int> &speeds);
    QList<int> &speedTable() { return m_speedTable; }
    const QList<int> &speedTable() const { return m_speedTable; }

    int currentQuestionIndex() const;

    // 兼容旧代码：weightedIndexArray 就是 m_indexArray
    const QList<int> &weightedIndexArray() const { return m_indexArray; }
    const QList<int> &indexArray() const { return m_indexArray; }

    int currentRoundSize() const { return m_roundCount; }
    int totalItems() const { return m_totalItems; }

private:
    void buildIndexArray();

    int m_totalItems = 0;
    int m_roundCount = 0;
    int m_currentIndex = -1;
    QList<int> m_speedTable;
    QList<int> m_indexArray;
};