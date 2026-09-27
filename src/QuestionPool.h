// QuestionPool.h
#pragma once
#include <QList>
#include <QRandomGenerator>

class QuestionPool
{
public:
    QuestionPool();

    // startBlockCursor: 跨会话恢复的块游标（>=0）。
    //   传入时若 >= blockCount 会被取模归位。
    void init(int totalItems,
              int roundCount,
              const QList<int> &speedTable = {},
              int startBlockCursor = 0);

    int next();
    int currentIndex() const { return m_currentIndex; }
    bool isRoundEnd() const;
    void reshuffle();          // 进入下一轮：换下一块
    void reset();

    void setSpeedTable(const QList<int> &speeds);
    QList<int> &speedTable() { return m_speedTable; }
    const QList<int> &speedTable() const { return m_speedTable; }

    int currentQuestionIndex() const;

    const QList<int> &weightedIndexArray() const { return m_indexArray; }
    const QList<int> &indexArray() const { return m_indexArray; }

    int currentRoundSize() const { return m_roundCount; }
    int totalItems() const { return m_totalItems; }

    // 跨会话进度：当前块游标 / 总块数
    int blockCursor() const { return m_blockCursor; }
    int blockCount() const { return m_blockCount; }

private:
    void buildIndexArray();        // 全量排序 + 计算块数（游标归零）
    void buildCurrentBlock();      // 取当前块 + 块内打乱

    int m_totalItems = 0;
    int m_roundCount = 0;
    int m_currentIndex = -1;
    QList<int> m_speedTable;

    QList<int> m_sortedAll;        // 按速度降序的全部索引
    int m_blockCursor = 0;         // 当前取到第几块（0-based）
    int m_blockCount = 0;          // 总块数

    QList<int> m_indexArray;       // 当前块的题目索引（已打乱）
};