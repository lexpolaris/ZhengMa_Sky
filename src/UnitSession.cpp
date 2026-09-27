// UnitSession.cpp
#include "UnitSession.h"
#include "ZmmbTable.h"
#include "Judge.h"
#include <QDateTime>
#include <QDebug>

UnitSession::UnitSession(QObject *parent) : QObject(parent) {}

void UnitSession::start(ZbUnit *unit, ZmmbTable *zmmb, int trainMax)
{
    m_unit = unit;
    m_zmmb = zmmb;
    m_trainMax = qMax(1, trainMax);
    m_currentRound = 0;
    m_testMode = false;

    if (!m_unit) return;

    const int count = m_unit->items.size();
    if (count <= 0) {
        qWarning() << "单元没有题目:" << m_unit->libName;
        return;
    }

    // 每轮题数（对应原版 sub_411B58）
    const int roundCount = [&]() {
        const int libNo = m_unit->libNo;
        if (libNo < 200) {
            if (count < 15)  return count;
            if (count < 100) return count / 2;
            if (count < 200) return count / 3;
            if (count < 300) return count / 3;
            if (count < 500) return count / 4;
            return 200;
        }
        return 50;
    }();

    // 速度表长度必须 = 题目数
    if (m_unit->speedTable.size() != count) {
        m_unit->speedTable.resize(count);
        for (int i = 0; i < count; ++i)
            m_unit->speedTable[i] = 0;
    }

    // 传入持久化的块游标，跨会话续上进度
    m_pool.init(count, qMax(1, roundCount),
                m_unit->speedTable,
                m_unit->poolBlockCursor);

    m_speed.setEffectiveTiming(true);   // 训练：有效计时
    m_speed.startSession();
    m_lastKeyTime = QDateTime::currentMSecsSinceEpoch();

    m_pool.next();
    emit questionChanged();
}

const ZbItem *UnitSession::currentItem() const
{
    if (!m_unit) return nullptr;
    const int idx = m_pool.currentQuestionIndex();
    if (idx < 0 || idx >= m_unit->items.size()) return nullptr;
    return &m_unit->items[idx];
}

QStringList UnitSession::getCorrectCodes(const ZbItem &item) const
{
    if (!item.code.isEmpty())
        return QStringList{item.code};
    if (m_zmmb)
        return m_zmmb->lookup(item.charText);
    return QStringList();
}

bool UnitSession::submit(const QString &input)
{
    if (!m_unit) return false;
    const ZbItem *item = currentItem();
    if (!item) return false;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 elapsed = now - m_lastKeyTime;
    m_lastKeyTime = now;

    const QStringList codes = getCorrectCodes(*item);
    if (codes.isEmpty()) {
        qDebug() << "未找到编码:" << item->charText;
        return false;
    }

    const bool correct = Judge::isCorrect(input, codes);

    // 更新单元计数
    if (correct) {
        ++m_unit->rightCount;
    } else {
        ++m_unit->wrongCount;
    }

    const int questionIndex = m_pool.currentQuestionIndex();
    updateSpeedTable(questionIndex, correct, elapsed, input.trimmed().length());
    // 字词数按题目个数计（每字/词计 1 个）；击键数按输入编码字符数计
    const int keyStrokes = qMax(1, input.trimmed().length());
    m_speed.recordKey(correct, 1, elapsed, keyStrokes);

    // 积分累计（对应原版 [self+62070] += [self+62074]）
    //   仅答对时累加，LibNo < 20 → 固定 3 分/题
    if (correct)
        m_speed.addScore(scoreForItem(m_unit->libNo));

    // 实时刷新 Used（每答一题检查一次）
    m_unit->used = UserData::isUnitCompleted(*m_unit) ? 1 : 0;

    m_unit->allPassed = UserData::isUnitAllPassed(*m_unit) ? 1 : 0;

    return correct;
}

void UnitSession::updateSpeedTable(int questionIndex, bool correct,
                                   qint64 elapsedMs, int codeLen)
{
    if (!m_unit) return;
    if (questionIndex < 0 || questionIndex >= m_unit->speedTable.size()) return;

    auto &table = m_unit->speedTable;

    if (elapsedMs < Judge::kMaxElapsedMs) {
        if (correct) {
            // 仍在错题区：先递减（错题多练几次，防止忘记）
            if (table[questionIndex] >= Judge::kPassMark)
                --table[questionIndex];

            // 已脱离错题区：写实际耗时
            if (table[questionIndex] < Judge::kPassMark) {
                // 按输入编码长度折算耗时（对应原版系数）：
                //   2 码 → ×0.7   3 码 → ×0.5   4 码 → ×0.3   其他 → ×1.0
                double factor = 1.0;
                switch (codeLen) {
                case 2:  factor = 0.7; break;
                case 3:  factor = 0.5; break;
                case 4:  factor = 0.3; break;
                default: factor = 1.0; break;
                }
                table[questionIndex] =
                    static_cast<int>(elapsedMs * factor + 0.5);
            }
        } else {
            // 答错：打错题标记
            table[questionIndex] = Judge::kErrorMark;
        }
    }

    m_pool.setSpeedTable(table);
}

// 每题积分（对应原版 [self+62074]）：LibNo < 20 → 固定 3 分/题
int UnitSession::scoreForItem(int libNo) const
{
    if (libNo < 20)
        return 3;
    return 1;
}

bool UnitSession::isRoundEnd() const
{
    return m_pool.isRoundEnd();
}

bool UnitSession::isAllRoundsEnd() const
{
    return m_currentRound >= m_trainMax - 1;
}

void UnitSession::nextRound()
{
    ++m_currentRound;
    m_pool.reshuffle();

    // 把新的块游标写回单元，保证跨会话续上
    if (m_unit)
        m_unit->poolBlockCursor = m_pool.blockCursor();

    m_pool.next();          // 立即取出新一轮第一题
    m_lastKeyTime = QDateTime::currentMSecsSinceEpoch();
    emit questionChanged();
}

void UnitSession::enterTestMode()
{
    m_testMode = true;
    m_speed.setEffectiveTiming(false);  // 测试：秒表计时（不停）
    m_speed.startSession();
    m_lastKeyTime = QDateTime::currentMSecsSinceEpoch();
    emit questionChanged();
}

int UnitSession::next()
{
    const int idx = m_pool.next();
    if (idx < 0) {
        // 不应发生：说明调用方在轮末还调 next()
        qWarning() << "QuestionPool::next() 越界，忽略";
        return -1;
    }
    m_lastKeyTime = QDateTime::currentMSecsSinceEpoch();
    emit questionChanged();
    return idx;
}

int UnitSession::calcTailTestItems(int count) const
{
    if (m_testItemsOverride >= 0)
        return m_testItemsOverride;
    if (count < 50)  return 2 * count;
    if (count < 400) return count;
    return 300;
}

QList<QPair<QString, QString>> UnitSession::getTestItems() const
{
    QList<QPair<QString, QString>> result;
    if (!m_unit) return result;

    const int count = m_unit->items.size();
    const int roundCount = calcTailTestItems(count);
    const auto &weighted = m_pool.weightedIndexArray();

    for (int i = 0; i < qMin(roundCount, weighted.size()); ++i) {
        const int idx = weighted[i];
        if (idx < 0 || idx >= m_unit->items.size()) continue;

        const ZbItem &item = m_unit->items[idx];
        QString code = item.code;
        if (code.isEmpty() && m_zmmb)
            code = m_zmmb->lookup(item.charText).join(' ');

        result.append({item.charText, code});
    }
    return result;
}

void UnitSession::reportTestAnswer(int questionIndex, bool correct)
{
    if (!m_unit) return;
    if (questionIndex < 0 || questionIndex >= m_unit->items.size()) return;
    if (questionIndex >= m_unit->speedTable.size()) return;

    auto &table = m_unit->speedTable;

    if (correct) {
        // 测试答对：直接脱离错题区（写 kPassMark - 1，非零且 < kPassMark）
        // 若从未练过（0），也视为通过
        if (table[questionIndex] >= Judge::kPassMark
            || table[questionIndex] == 0) {
            table[questionIndex] = Judge::kPassMark - 1;
        }
    } else {
        // 答错：标记错题
        table[questionIndex] = Judge::kErrorMark;
    }
}

QList<UnitSession::TestItemEntry> UnitSession::getTestItemsWithIndex() const
{
    QList<TestItemEntry> result;
    if (!m_unit) return result;

    const int count = m_unit->items.size();
    const int roundCount = calcTailTestItems(count);
    const auto &weighted = m_pool.weightedIndexArray();

    for (int i = 0; i < qMin(roundCount, weighted.size()); ++i) {
        const int idx = weighted[i];
        if (idx < 0 || idx >= m_unit->items.size()) continue;

        const ZbItem &item = m_unit->items[idx];
        QString code = item.code;
        if (code.isEmpty() && m_zmmb)
            code = m_zmmb->lookup(item.charText).join(' ');

        result.append({item.charText, code, idx});
    }
    return result;
}
