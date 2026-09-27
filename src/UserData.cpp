// UserData.cpp
#include "UserData.h"

#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>

UserData::UserData() {}

// ---------------------------------------------------------------------------
// 题库加载（静态数据，只读）
//
// 根据扩展名自动选择解析器：
//   .txt → 紧凑文本格式（新，推荐）
//   其他 → 旧版 XML（向后兼容）
// ---------------------------------------------------------------------------
bool UserData::loadTemplate(const QString &filePath)
{
    if (filePath.endsWith(".txt", Qt::CaseInsensitive))
        return loadTemplateText(filePath);
    return loadTemplateXml(filePath);
}

// ---------------------------------------------------------------------------
// 紧凑文本题库解析（train.txt）
//
// 格式（UTF-8，制表符分隔）：
//   单元头:  @Lib<TAB>LibNo<TAB>LibName<TAB>Count
//   题目行:  字词<TAB>编码<TAB>拆分<TAB>联想
//   空行 / 以 '#' 开头的行  → 忽略
//
// 与旧版 XML 相比：去掉了用户进度/配置属性，本文件只含静态题库，
// 体积更小、便于手工维护与 diff；进度与速度表一律存于 user.xml。
// ---------------------------------------------------------------------------
bool UserData::loadTemplateText(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开题库:" << filePath;
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    m_units.clear();

    ZbUnit *unit = nullptr;   // 当前正在填充的单元
    int lineNo = 0;

    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        ++lineNo;

        // 跳过空行与注释
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        // 单元头：@Lib \t LibNo \t LibName \t Count
        if (line.startsWith("@Lib")) {
            const QStringList cols = line.split('\t');
            if (cols.size() < 3) {
                qWarning() << "题库第" << lineNo << "行单元头格式错误";
                unit = nullptr;
                continue;
            }
            ZbUnit u;
            u.libNo   = cols.value(1).toInt();
            u.libName = cols.value(2);
            m_units.append(u);
            unit = &m_units.last();
            continue;
        }

        // 题目行：字词 \t 编码 \t 拆分 \t 联想（必须先有单元头）
        if (!unit)
            continue;

        const QStringList cols = line.split('\t');
        ZbItem item;
        item.charText  = cols.value(0);
        item.code      = cols.value(1);
        item.split     = cols.value(2);
        item.associate = cols.value(3);

        if (item.charText.trimmed().isEmpty())
            continue;

        unit->items.append(item);
    }

    file.close();

    // 收尾：补全题目数并初始化速度表 / 索引数组
    for (ZbUnit &u : m_units)
        finalizeUnit(u);

    qDebug() << "题库(文本)加载完成:" << m_units.size() << "个单元";
    return !m_units.isEmpty();
}

// ---------------------------------------------------------------------------
// 旧版 XML 题库解析（Train.xml），保留用于向后兼容
// ---------------------------------------------------------------------------
bool UserData::loadTemplateXml(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开模板:" << filePath;
        return false;
    }

    QDomDocument doc;
    const auto result = doc.setContent(&file);
    file.close();
    if (!result) {
        qWarning() << "模板解析失败:" << result.errorMessage;
        return false;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != "ZmSoft") return false;

    // 只读单元和题目
    m_units.clear();
    QDomElement libs = root.firstChildElement("Libs");
    for (QDomElement libElem = libs.firstChildElement("Lib");
         !libElem.isNull();
         libElem = libElem.nextSiblingElement("Lib"))
    {
        ZbUnit unit;
        parseUnit(libElem, unit);
        m_units.append(unit);
    }

    qDebug() << "题库(XML)加载完成:" << m_units.size() << "个单元";
    return true;
}

// 补全单元的派生字段：题目数、速度表、索引数组（两个解析器共用）
void UserData::finalizeUnit(ZbUnit &unit)
{
    unit.count = unit.items.size();

    // 速度表（0 = 未训练，越大越慢）
    unit.speedTable.assign(unit.items.size(), 0);

    // 题目池索引
    unit.indexArray.resize(unit.items.size());
    for (int i = 0; i < unit.items.size(); ++i)
        unit.indexArray[i] = i;
}

bool UserData::loadUserData(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "用户数据不存在，将使用模板默认值:" << filePath;
        return false;   // 不算错误
    }

    QDomDocument doc;
    const auto result = doc.setContent(&file);
    file.close();
    if (!result) {
        qWarning() << "用户数据解析失败:" << result.errorMessage;
        return false;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != "ZmSoft") return false;

    // 1. 用户配置
    m_statusName        = root.attribute("StatusName", m_statusName);
    m_score             = root.attribute("Score", "0").toInt();
    m_date              = root.attribute("Date", m_date);
    m_displayType       = root.attribute("DisplayType", "0").toInt();
    m_tailTrainMax      = root.attribute("TailTrainMax", "3").toInt();

    m_showSimilarRoot   = root.attribute("ShowSimilarRoot", "0").toInt();
    m_windowsLayout     = root.attribute("WindowsLayout", "0").toInt();
    m_autoTailTrainCount= root.attribute("AutoTailTrainCount", "0").toInt();
    m_showHitSpeed      = root.attribute("ShowHitSpeed", "1").toInt();
    m_lastLibNo         = root.attribute("LastLibNo", "-1").toInt();
    m_lastMode          = root.attribute("LastMode", "-1").toInt();

    // 2. 模式状态
    QDomElement trainElem = root.firstChildElement("Train");
    if (!trainElem.isNull()) parseModeState(trainElem, m_train);

    QDomElement testElem = root.firstChildElement("Test");
    if (!testElem.isNull()) parseModeState(testElem, m_test);

    // 3. 单元进度 + 速度表
    QDomElement libs = root.firstChildElement("Libs");
    for (QDomElement libElem = libs.firstChildElement("Lib");
         !libElem.isNull();
         libElem = libElem.nextSiblingElement("Lib"))
    {
        const int libNo = libElem.attribute("LibNo", "-1").toInt();
        ZbUnit *unit = findUnit(libNo);
        if (!unit) continue;

        // 进度
        unit->wrongCount = libElem.attribute("WrongCount", "0").toInt();
        unit->rightCount = libElem.attribute("RightCount", "0").toInt();
        unit->used       = libElem.attribute("Used", "0").toInt();
        unit->poolBlockCursor = libElem.attribute("PoolBlockCursor", "0").toInt();
        unit->allPassed = libElem.attribute("AllPassed", "0").toInt();

        // 速度表
        QDomElement speeds = libElem.firstChildElement("SpeedTable");
        if (!speeds.isNull()) {
            // 先全部置 0
            unit->speedTable.resize(unit->items.size());
            for (int i = 0; i < unit->speedTable.size(); ++i)
                unit->speedTable[i] = 0;

            for (QDomElement s = speeds.firstChildElement("s");
                !s.isNull();
                s = s.nextSiblingElement("s"))
            {
                const int idx = s.attribute("i", "-1").toInt();
                const int val = s.attribute("v", "0").toInt();
                if (idx >= 0 && idx < unit->speedTable.size())
                    unit->speedTable[idx] = val;
            }
        }

        // 根据速度表刷新 Used
        unit->used = isUnitCompleted(*unit) ? 1 : 0;
    }

    qDebug() << "用户数据加载完成";
    return true;
}

bool UserData::saveUserData(const QString &filePath) const
{
    QDomDocument doc;
    QDomProcessingInstruction pi = doc.createProcessingInstruction(
        "xml", "version='1.0' encoding='utf-8'");
    doc.appendChild(pi);

    QDomElement root = doc.createElement("ZmSoft");
    root.setAttribute("StatusName", m_statusName);
    root.setAttribute("Score", QString::number(m_score));
    root.setAttribute("Date", m_date);
    root.setAttribute("DisplayType", QString::number(m_displayType));
    root.setAttribute("TailTrainMax", QString::number(m_tailTrainMax));
    root.setAttribute("ShowSimilarRoot", QString::number(m_showSimilarRoot));
    root.setAttribute("WindowsLayout", QString::number(m_windowsLayout));
    root.setAttribute("AutoTailTrainCount", QString::number(m_autoTailTrainCount));
    root.setAttribute("ShowHitSpeed", QString::number(m_showHitSpeed));
    root.setAttribute("LastLibNo", QString::number(m_lastLibNo));
    root.setAttribute("LastMode", QString::number(m_lastMode));
    doc.appendChild(root);

    // 模式状态
    auto writeMode = [&](const QString &tag, const ZbModeState &s) {
        QDomElement e = doc.createElement(tag);
        e.setAttribute("LibName", s.libName);
        e.setAttribute("Grade", QString::number(s.grade));
        e.setAttribute("Xp", QString::number(s.xp));
        e.setAttribute("Streak", QString::number(s.streak));
        e.setAttribute("WrongCount", QString::number(s.wrongCount));
        e.setAttribute("RightCount", QString::number(s.rightCount));
        e.setAttribute("TotalTime", QString::number(s.totalTime));
        e.setAttribute("Time", QString::number(s.time));
        e.setAttribute("RoundCount", QString::number(s.roundCount));
        root.appendChild(e);
    };
    writeMode("Train", m_train);
    writeMode("Test",  m_test);

    // 单元进度 + 速度表
    QDomElement libs = doc.createElement("Libs");
    for (const ZbUnit &u : m_units) {
        QDomElement lib = doc.createElement("Lib");
        lib.setAttribute("LibName", u.libName);
        lib.setAttribute("LibNo", QString::number(u.libNo));
        lib.setAttribute("Count", QString::number(u.count));
        lib.setAttribute("WrongCount", QString::number(u.wrongCount));
        lib.setAttribute("RightCount", QString::number(u.rightCount));

        // Used 始终根据速度表实时计算，避免与 u.used 不一致
        lib.setAttribute("Used", isUnitCompleted(u) ? "1" : "0");

        lib.setAttribute("PoolBlockCursor", QString::number(u.poolBlockCursor));
        lib.setAttribute("AllPassed", u.allPassed ? "1" : "0");

        // 速度表：只保存非零项，压缩存储
        QDomElement speeds = doc.createElement("SpeedTable");
        for (int i = 0; i < u.speedTable.size(); ++i) {
            const int s = u.speedTable[i];
            if (s == 0) continue;   // 0 表示未训练，跳过
            QDomElement se = doc.createElement("s");
            se.setAttribute("i", QString::number(i));
            se.setAttribute("v", QString::number(s));
            speeds.appendChild(se);
        }
        if (speeds.hasChildNodes())
            lib.appendChild(speeds);

        // 题目本身保存在题库文件（train.txt / Train.xml）中，
        // user.xml 只存进度与速度表，不重复保存题目。
        libs.appendChild(lib);
    }
    root.appendChild(libs);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法写入用户数据:" << filePath;
        return false;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << doc.toString(2);
    file.close();
    return true;
}

void UserData::parseRoot(const QDomElement &root)
{
    // 用户配置属性
    m_statusName        = root.attribute("StatusName", "Train");
    m_score             = root.attribute("Score", "0").toInt();
    m_date              = root.attribute("Date");
    m_displayType       = root.attribute("DisplayType", "0").toInt();
    m_tailTrainMax      = root.attribute("TailTrainMax", "3").toInt();
    m_showSimilarRoot   = root.attribute("ShowSimilarRoot", "0").toInt();
    m_windowsLayout     = root.attribute("WindowsLayout", "0").toInt();
    m_autoTailTrainCount= root.attribute("AutoTailTrainCount", "0").toInt();
    m_showHitSpeed      = root.attribute("ShowHitSpeed", "1").toInt();
    m_lastLibNo         = root.attribute("LastLibNo", "0").toInt();
    m_lastMode          = root.attribute("LastMode",  "0").toInt();

    // 遍历子节点
    for (QDomElement elem = root.firstChildElement();
         !elem.isNull();
         elem = elem.nextSiblingElement())
    {
        const QString tag = elem.tagName();
        if (tag == "Train") {
            parseModeState(elem, m_train);
        } else if (tag == "Test") {
            parseModeState(elem, m_test);
        } else if (tag == "Libs") {
            for (QDomElement libElem = elem.firstChildElement("Lib");
                 !libElem.isNull();
                 libElem = libElem.nextSiblingElement("Lib"))
            {
                ZbUnit unit;
                parseUnit(libElem, unit);
                m_units.append(unit);
            }
        }
    }

    qDebug() << "已加载" << m_units.size() << "个单元";
}

void UserData::parseModeState(const QDomElement &elem, ZbModeState &state)
{
    state.libName    = elem.attribute("LibName");
    state.grade      = elem.attribute("Grade", "0").toInt();
    state.xp         = elem.attribute("Xp", "0").toLongLong();
    state.streak     = elem.attribute("Streak", "0").toInt();
    state.wrongCount = elem.attribute("WrongCount", "0").toInt();
    state.rightCount = elem.attribute("RightCount", "0").toInt();
    state.totalTime  = elem.attribute("TotalTime", "0").toInt();
    state.time       = elem.attribute("Time", "0").toInt();
    state.roundCount = elem.attribute("RoundCount", "0").toInt();
}

void UserData::parseUnit(const QDomElement &elem, ZbUnit &unit)
{
    unit.libName    = elem.attribute("LibName");
    unit.libNo      = elem.attribute("LibNo", "0").toInt();
    unit.count      = elem.attribute("Count", "0").toInt();
    unit.wrongCount = elem.attribute("WrongCount", "0").toInt();
    unit.rightCount = elem.attribute("RightCount", "0").toInt();
    unit.used       = elem.attribute("Used", "0").toInt();

    for (QDomElement b = elem.firstChildElement("b");
         !b.isNull();
         b = b.nextSiblingElement("b"))
    {
        ZbItem item;
        item.charText  = b.attribute("c");
        item.code      = b.attribute("m");
        item.split     = b.attribute("s");
        item.associate = b.attribute("a");

        if (item.charText.trimmed().isEmpty())
            continue;

        unit.items.append(item);
    }

    // 初始化速度表 / 索引数组（统一走 finalizeUnit）
    finalizeUnit(unit);
}

ZbUnit* UserData::findUnit(int libNo)
{
    for (auto &u : m_units)
        if (u.libNo == libNo) return &u;
    return nullptr;
}

bool UserData::isUnitCompleted(const ZbUnit &unit)
{
    if (unit.items.isEmpty()) return false;

    int trained = 0;
    for (int i = 0; i < unit.speedTable.size() && i < unit.items.size(); ++i) {
        if (unit.speedTable[i] != 0)
            ++trained;
    }
    return trained >= unit.items.size();
}

// 单元是否"每道题都答对过"：
//   速度表值 != 0（练过）且 < Judge::kPassMark（已脱离错题区）
bool UserData::isUnitAllPassed(const ZbUnit &unit)
{
    if (unit.items.isEmpty()) return false;

    const int n = unit.items.size();
    for (int i = 0; i < n; ++i) {
        const int v = (i < unit.speedTable.size()) ? unit.speedTable[i] : 0;
        if (v == 0 || v >= Judge::kPassMark)
            return false;
    }
    return true;
}