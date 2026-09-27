// Judge.h
#pragma once
#include <QString>
#include <QStringList>

class Judge
{
public:
    // 判定：输入是否匹配任一编码
    static bool isCorrect(const QString &input, const QStringList &codes);
    static bool isCorrect(const QString &input, const QString &code);

    // 错误标记值（速度表中）
    static constexpr int kErrorMark = 50002;

    // 脱离错题区的门槛：速度表值 < kPassMark 视为"已答对通过"
    // updateSpeedTable 中：答错→kErrorMark，答对逐步 --，
    // 降到 < kPassMark 时写实际耗时，脱离错题区。
    static constexpr int kPassMark  = 50000;

    static constexpr int kMaxElapsedMs = 20000;
};