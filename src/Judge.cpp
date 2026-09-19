// Judge.cpp
#include "Judge.h"

bool Judge::isCorrect(const QString &input, const QStringList &codes)
{
    if (input.isEmpty() || codes.isEmpty())
        return false;

    const QString inputWrapped = " " + input.trimmed() + " ";

    for (const QString &code : codes) {
        if (code.isEmpty()) continue;
        const QString correctWrapped = " " + code.trimmed() + " ";
        if (correctWrapped.contains(inputWrapped, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool Judge::isCorrect(const QString &input, const QString &code)
{
    return isCorrect(input, QStringList{code});
}

double Judge::lengthFactor(int targetLength)
{
    switch (targetLength) {
    case 2: return 0.7;
    case 3: return 0.5;
    case 4: return 0.3;
    default: return 1.0;
    }
}