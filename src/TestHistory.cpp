// TestHistory.cpp
#include "TestHistory.h"
#include <QFile>
#include <QDomDocument>
#include <QTextStream>
#include <QDebug>

TestHistory::TestHistory() {}

bool TestHistory::load(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "测试记录文件不存在，将创建新的";
        return true;   // 不算错误
    }

    QDomDocument doc;
    const auto result = doc.setContent(&file);
    file.close();

    if (!result) {
        qWarning() << "测试记录解析失败:" << result.errorMessage;
        return false;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != "TestHistory") return false;

    m_records.clear();
    for (QDomElement e = root.firstChildElement("Record");
         !e.isNull();
         e = e.nextSiblingElement("Record"))
    {
        TestRecord rec;
        rec.libNo      = e.attribute("LibNo", "0").toInt();
        rec.libName    = e.attribute("LibName");
        rec.speed      = e.attribute("Speed", "0").toInt();
        rec.bestSpeed  = e.attribute("BestSpeed", "0").toInt();
        rec.hitSpeed   = e.attribute("HitSpeed", "0").toDouble();
        rec.accuracy   = e.attribute("Accuracy", "0").toInt();
        rec.wrongCount = e.attribute("WrongCount", "0").toInt();
        rec.totalMs    = e.attribute("TotalTime", "0").toLongLong();
        rec.dateTime   = QDateTime::fromString(
            e.attribute("Date"), Qt::ISODate);
        m_records.append(rec);
    }

    qDebug() << "已加载" << m_records.size() << "条测试记录";
    return true;
}

bool TestHistory::save(const QString &filePath) const
{
    QDomDocument doc;
    QDomProcessingInstruction pi = doc.createProcessingInstruction(
        "xml", "version='1.0' encoding='utf-8'");
    doc.appendChild(pi);

    QDomElement root = doc.createElement("TestHistory");
    doc.appendChild(root);

    for (const TestRecord &rec : m_records) {
        QDomElement e = doc.createElement("Record");
        e.setAttribute("LibNo", rec.libNo);
        e.setAttribute("LibName", rec.libName);
        e.setAttribute("Speed", rec.speed);
        e.setAttribute("BestSpeed", rec.bestSpeed);
        e.setAttribute("HitSpeed", QString::number(rec.hitSpeed, 'f', 2));
        e.setAttribute("Accuracy", rec.accuracy);
        e.setAttribute("WrongCount", rec.wrongCount);
        e.setAttribute("TotalTime", QString::number(rec.totalMs));
        e.setAttribute("Date", rec.dateTime.toString(Qt::ISODate));
        root.appendChild(e);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法写入测试记录:" << filePath;
        return false;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << doc.toString(2);
    file.close();
    return true;
}

void TestHistory::addRecord(const TestRecord &rec)
{
    m_records.prepend(rec);   // 新记录放最前

    // 只保留每个单元最近 kMaxRecords 条
    QList<TestRecord> filtered;
    QHash<int, int> countByUnit;
    for (const TestRecord &r : m_records) {
        if (countByUnit.value(r.libNo, 0) < kMaxRecords) {
            filtered.append(r);
            countByUnit[r.libNo]++;
        }
    }
    m_records = filtered;
}

QList<TestRecord> TestHistory::recordsByUnit(int libNo) const
{
    QList<TestRecord> result;
    for (const TestRecord &r : m_records)
        if (r.libNo == libNo) result.append(r);
    return result;
}