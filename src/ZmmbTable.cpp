// ZmmbTable.cpp
#include "ZmmbTable.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

ZmmbTable::ZmmbTable() {}

bool ZmmbTable::load(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开码表:" << filePath;
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    int lineNo = 0;
    int count = 0;
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        ++lineNo;

        if (line.isEmpty()) continue;

        // 按 Tab 分割
        const int tabPos = line.indexOf('\t');
        if (tabPos < 0) continue;

        const QString word = line.left(tabPos).trimmed();
        QString codeStr = line.mid(tabPos + 1).trimmed();

        if (word.isEmpty() || codeStr.isEmpty()) continue;

        // 多个编码用空格分隔
        QStringList codes = codeStr.split(' ', Qt::SkipEmptyParts);

        // 只取第一个 \n 前的内容（防止意外）
        if (!codes.isEmpty()) {
            // 去掉可能的 \x0A 后内容
            QString &first = codes[0];
            int nlPos = first.indexOf(QChar(0x0A));
            if (nlPos >= 0) first = first.left(nlPos);
        }

        m_map.insert(word, codes);
        ++count;
    }

    file.close();
    qDebug() << "码表加载完成: 共" << count << "条";
    return true;
}

QStringList ZmmbTable::lookup(const QString &word) const
{
    return m_map.value(word, QStringList());
}

bool ZmmbTable::contains(const QString &word) const
{
    return m_map.contains(word);
}