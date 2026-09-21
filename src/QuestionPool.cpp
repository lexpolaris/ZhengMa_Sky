// QuestionPool.cpp
#include "QuestionPool.h"
#include <algorithm>

QuestionPool::QuestionPool() {}

void QuestionPool::init(int totalItems,
                        int roundCount,
                        const QList<int> &speedTable)
{
    m_totalItems = qMax(0, totalItems);
    m_roundCount = qMax(0, roundCount);
    m_currentIndex = -1;

    // 速度表：长度必须 = 题目总数
    if (speedTable.size() == m_totalItems) {
        m_speedTable = speedTable;
    } else {
        m_speedTable.resize(m_totalItems);
        for (int i = 0; i < m_totalItems; ++i)
            m_speedTable[i] = 0;
    }

    buildIndexArray();
}

void QuestionPool::buildIndexArray()
{
    m_indexArray.clear();
    if (m_totalItems <= 0) return;

    // 1. 初始化索引数组 [0, 1, ..., N-1]
    m_indexArray.resize(m_totalItems);
    for (int i = 0; i < m_totalItems; ++i)
        m_indexArray[i] = i;

    // 2. 按速度排序（慢→快），只排前 roundCount 个
    //    对应原版冒泡排序：每轮把最慢的冒泡到前面
    const int n = qMin(m_totalItems, m_roundCount);
    for (int k = 0; k < n; ++k) {
        for (int m = m_totalItems - 1; m > k; --m) {
            const int sa = m_speedTable[m_indexArray[m]];
            const int sb = m_speedTable[m_indexArray[m - 1]];
            if (sa > sb)
                std::swap(m_indexArray[m], m_indexArray[m - 1]);
        }
    }

    // 3. Fisher-Yates 洗牌（只洗前 n 个）
    //    对应原版：
    //      v57 = (n + Random(401512 - n)) % 401476
    //      swap(IndexArray[n], IndexArray[v57])
    for (int i = 0; i < n; ++i) {
        const int remaining = m_roundCount - i;
        if (remaining <= 0) break;
        // Random(remaining) → [0, remaining)
        const int r = QRandomGenerator::global()->bounded(remaining);
        const int j = (i + r) % m_totalItems;
        std::swap(m_indexArray[i], m_indexArray[j]);
    }

    m_currentIndex = -1;
}

void QuestionPool::reshuffle()
{
    // 重新排序 + 洗牌（速度表可能变了）
    buildIndexArray();
}

void QuestionPool::reset()
{
    m_currentIndex = -1;
}

int QuestionPool::next()
{
    if (m_indexArray.isEmpty()) return -1;
    ++m_currentIndex;
    if (m_currentIndex >= m_indexArray.size()) {
        // 超出本轮题数，从头开始（防御）
        m_currentIndex = 0;
    }
    return m_indexArray[m_currentIndex];
}

bool QuestionPool::isRoundEnd() const
{
    if (m_roundCount <= 0) return true;
    return m_currentIndex >= m_roundCount - 1;
}

void QuestionPool::setSpeedTable(const QList<int> &speeds)
{
    if (speeds.size() == m_totalItems)
        m_speedTable = speeds;
}

int QuestionPool::currentQuestionIndex() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_indexArray.size())
        return -1;
    return m_indexArray[m_currentIndex];
}