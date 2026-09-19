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

    // 每轮题数
    const int roundCount = [&]() {
        const int libNo = m_unit->libNo;
        if (libNo < 200) {
            if (count < 15)  return count;
            if (count < 100) return count / 2;
            if (count < 200) return count / 3;
            if (count < 500) return count / 4;
            return 200;
        }
        return 50;
    }();

    m_pool.init(count, qMax(1, roundCount), m_unit->speedTable, 3);
    m_speed.startSession();
    m_lastKeyTime = QDateTime::currentMSecsSinceEpoch();

    m_pool.next();          // 取出第一题
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
    updateSpeedTable(questionIndex, correct, elapsed);
    m_speed.recordKey(correct, item->code.length(), elapsed);

    // 实时刷新 Used（每答一题检查一次）
    m_unit->used = UserData::isUnitCompleted(*m_unit) ? 1 : 0;

    return correct;
}

void UnitSession::updateSpeedTable(int questionIndex, bool correct, qint64 elapsedMs)
{
    if (!m_unit) return;
    if (questionIndex < 0 || questionIndex >= m_unit->speedTable.size()) return;

    auto &table = m_unit->speedTable;

    if (elapsedMs < Judge::kMaxElapsedMs) {
        if (correct) {
            const double factor = Judge::lengthFactor(
                m_unit->items[questionIndex].code.length());

            if (table[questionIndex] >= 50000)
                --table[questionIndex];

            if (table[questionIndex] < 50000)
                table[questionIndex] = static_cast<int>(elapsedMs * factor);
        } else {
            table[questionIndex] = Judge::kErrorMark;
        }
    }

    m_pool.setSpeedTable(table);
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
    m_pool.next();          // 立即取出新一轮第一题
    m_lastKeyTime = QDateTime::currentMSecsSinceEpoch();
    emit questionChanged();
}

void UnitSession::enterTestMode()
{
    m_testMode = true;
    m_speed.startSession();
    m_lastKeyTime = QDateTime::currentMSecsSinceEpoch();
    emit questionChanged();
}

int UnitSession::next()
{
    const int idx = m_pool.next();
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
        if (table[questionIndex] >= Judge::kErrorMark) {
            // 曾经错，现在对：清除错误标记
            table[questionIndex] = 0;
        }
    } else {
        // 答错：标记为错题，训练时加重
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