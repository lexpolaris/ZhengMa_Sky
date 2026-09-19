// ZmmbTable.h
#pragma once
#include <QString>
#include <QStringList>
#include <QHash>

class ZmmbTable
{
public:
    ZmmbTable();

    bool load(const QString &filePath);

    // 查询：返回编码列表（可能多个）
    QStringList lookup(const QString &word) const;

    // 是否包含
    bool contains(const QString &word) const;

    // 条目数
    int size() const { return m_map.size(); }

private:
    // 字/词 -> 编码列表
    QHash<QString, QStringList> m_map;
};