// Judge.h
#pragma once
#include <QString>
#include <QStringList>

class Judge
{
public:
    // 判定：输入是否匹配任一编码
    // input: 用户输入
    // codes: 正确答案（可能多个）
    static bool isCorrect(const QString &input, const QStringList &codes);

    // 判定（单个编码）
    static bool isCorrect(const QString &input, const QString &code);

    // 错误标记值（速度表中）
    static constexpr int kErrorMark = 50002;
    static constexpr int kMaxElapsedMs = 20000;
};