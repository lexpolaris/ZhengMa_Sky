// QuestionPool.cpp
#include "QuestionPool.h"
#include <algorithm>

QuestionPool::QuestionPool() {}

void QuestionPool::init(int totalItems,
                        int roundCount,
                        const QList<int> &speedTable,
                        int startBlockCursor)
{
    m_totalItems = qMax(0, totalItems);
    m_roundCount = qMax(1, roundCount);   // 至少 1，避免除零
    m_currentIndex = -1;

    if (speedTable.size() == m_totalItems) {
        m_speedTable = speedTable;
    } else {
        m_speedTable.resize(m_totalItems);
        for (int i = 0; i < m_totalItems; ++i)
            m_speedTable[i] = 0;
    }

    buildIndexArray();     // 排序 + 计算块数，游标归零

    // 跨会话恢复块游标：越界则取模归位
    if (m_blockCount > 0 && startBlockCursor > 0) {
        m_blockCursor = startBlockCursor % m_blockCount;
    } else {
        m_blockCursor = 0;
    }

    buildCurrentBlock();   // 取恢复后的那一块
}

// 全量排序（速度降序：慢/错题在前），计算块数，游标归零
void QuestionPool::buildIndexArray()
{
    m_sortedAll.clear();
    m_indexArray.clear();
    m_blockCursor = 0;
    m_currentIndex = -1;

    if (m_totalItems <= 0) {
        m_blockCount = 0;
        return;
    }

    m_sortedAll.reserve(m_totalItems);
    for (int i = 0; i < m_totalItems; ++i)
        m_sortedAll.append(i);

    // 速度表值大 = 慢/错，排前面
    std::stable_sort(m_sortedAll.begin(), m_sortedAll.end(),
        [this](int a, int b) {
            return m_speedTable[a] > m_speedTable[b];
        });

    // 块数 = ceil(totalItems / roundCount)
    m_blockCount = (m_totalItems + m_roundCount - 1) / m_roundCount;
}

// 取当前块：从 m_sortedAll 里切出 [cursor*roundCount, +roundCount)，
// 块内 Fisher-Yates 打乱
void QuestionPool::buildCurrentBlock()
{
    m_indexArray.clear();
    m_currentIndex = -1;

    if (m_totalItems <= 0 || m_blockCount <= 0) return;

    const int start = m_blockCursor * m_roundCount;
    const int end   = qMin(start + m_roundCount, m_totalItems);

    for (int i = start; i < end; ++i)
        m_indexArray.append(m_sortedAll[i]);

    // 块内打乱
    const int n = m_indexArray.size();
    for (int i = 0; i < n; ++i) {
        const int r = QRandomGenerator::global()->bounded(n - i);
        std::swap(m_indexArray[i], m_indexArray[i + r]);
    }
}

// 进入下一轮：游标前进；走完所有块后重新排序分块（速度表已更新）
void QuestionPool::reshuffle()
{
    if (m_totalItems <= 0) return;

    ++m_blockCursor;

    if (m_blockCursor >= m_blockCount) {
        // 所有块都练过一遍：重新按最新速度表排序分块
        buildIndexArray();     // 游标归零
    }

    buildCurrentBlock();
}

void QuestionPool::reset()
{
    m_currentIndex = -1;
}

int QuestionPool::next()
{
    if (m_indexArray.isEmpty()) return -1;

    // 越界保护：返回 -1，让调用方明确处理（不再卡在最后一题）
    if (m_currentIndex + 1 >= m_indexArray.size())
        return -1;

    ++m_currentIndex;
    return m_indexArray[m_currentIndex];
}

bool QuestionPool::isRoundEnd() const
{
    if (m_indexArray.isEmpty()) return true;
    return m_currentIndex >= m_indexArray.size() - 1;
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