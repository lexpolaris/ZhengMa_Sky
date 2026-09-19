// QuestionPool.cpp
#include "QuestionPool.h"
#include "Judge.h"

#include <algorithm>
#include <QRandomGenerator>

QuestionPool::QuestionPool() {}

void QuestionPool::init(int totalItems,
                        int roundCount,
                        const QList<int> &speedTable,
                        int errorRepeat)
{
    m_totalItems = totalItems;
    m_roundCount = roundCount;
    m_currentIndex = -1;

    if (!speedTable.isEmpty() && speedTable.size() == totalItems)
        m_speedTable = speedTable;
    else if (m_speedTable.size() != totalItems) {
        m_speedTable.resize(totalItems);
        for (int i = 0; i < totalItems; ++i)
            m_speedTable[i] = 0;
    }

    buildWeightedPool(errorRepeat);
    reshuffle();          // 统一在这里洗牌
}

void QuestionPool::sortBySpeed()
{
    std::stable_sort(m_weightedIndex.begin(), m_weightedIndex.end(),
        [this](int a, int b) {
            const int sa = (a < m_speedTable.size()) ? m_speedTable[a] : 0;
            const int sb = (b < m_speedTable.size()) ? m_speedTable[b] : 0;
            return sa > sb;
        });
}

void QuestionPool::buildWeightedPool(int errorRepeat)
{
    m_weightedIndex.clear();
    if (errorRepeat < 1) errorRepeat = 1;

    for (int i = 0; i < m_speedTable.size(); ++i) {
        const int s = m_speedTable[i];
        if (s >= Judge::kErrorMark) {
            for (int k = 0; k < errorRepeat; ++k)
                m_weightedIndex.append(i);
        } else {
            m_weightedIndex.append(i);
        }
    }

    // 加权池必须至少覆盖一轮题数
    if (m_roundCount > 0 && m_weightedIndex.size() < m_roundCount) {
        // 用循环补齐（防止题目数太少导致一轮取不满）
        const int base = m_weightedIndex.size();
        if (base > 0) {
            int i = 0;
            while (m_weightedIndex.size() < m_roundCount) {
                m_weightedIndex.append(m_weightedIndex[i % base]);
                ++i;
            }
        }
    }
}

void QuestionPool::shuffle()
{
    for (int i = m_weightedIndex.size() - 1; i > 0; --i) {
        const int j = QRandomGenerator::global()->bounded(i + 1);
        std::swap(m_weightedIndex[i], m_weightedIndex[j]);
    }
}

void QuestionPool::reshuffle()
{
    buildWeightedPool(3);
    shuffle();
    m_currentIndex = -1;
}

int QuestionPool::next()
{
    if (m_weightedIndex.isEmpty()) return -1;
    ++m_currentIndex;
    if (m_currentIndex >= m_weightedIndex.size())
        m_currentIndex = 0;
    return m_weightedIndex[m_currentIndex];
}

bool QuestionPool::isRoundEnd() const
{
    // 一轮结束：当前索引已经到达 roundCount-1（或超过）
    return m_currentIndex >= m_roundCount - 1;
}

void QuestionPool::reset()
{
    m_currentIndex = -1;
}

void QuestionPool::setSpeedTable(const QList<int> &speeds)
{
    m_speedTable = speeds;
}

int QuestionPool::currentQuestionIndex() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_weightedIndex.size())
        return -1;
    return m_weightedIndex[m_currentIndex];
}