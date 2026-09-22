// UserData.h
#pragma once
#include <QString>
#include <QList>
#include <QDomDocument>

// 题目
struct ZbItem {
    QString charText;    // c = 字/词/字根
    QString code;        // m = 编码（可能为空）
    QString split;       // s = 拆分
    QString associate;   // a = 联想提示
};

// 单元
struct ZbUnit {
    QString libName;
    int libNo = 0;
    int count = 0;
    int wrongCount = 0;
    int rightCount = 0;
    int used = 0;
    QList<ZbItem> items;

    // 运行时状态
    QList<int> speedTable;   // 每题速度（ms）
    QList<int> indexArray;   // 题目池索引
};

// 模式状态
struct ZbModeState {
    QString libName;
    int grade = 0;
    int wrongCount = 0;
    int rightCount = 0;
    int totalTime = 0;
    int time = 0;
    int roundCount = 0;
};

class UserData
{
public:
    UserData();

    // 加载模板（只读）
    bool loadTemplate(const QString &filePath);

    // 加载用户数据（可写），如果不存在就返回 false
    bool loadUserData(const QString &filePath);

    // 保存用户数据
    bool saveUserData(const QString &filePath) const;

    // 用户配置
    QString statusName() const { return m_statusName; }
    int score() const { return m_score; }
    QString date() const { return m_date; }
    int displayType() const { return m_displayType; }
    int tailTrainMax() const { return m_tailTrainMax; }

    int showSimilarRoot() const { return m_showSimilarRoot; }
    int windowsLayout() const { return m_windowsLayout; }
    int autoTailTrainCount() const { return m_autoTailTrainCount; }
    int showHitSpeed() const { return m_showHitSpeed; }

    // 上次状态
    int lastLibNo() const { return m_lastLibNo; }
    int lastMode()  const { return m_lastMode;  }
    void setLastLibNo(int v) { m_lastLibNo = v; }
    void setLastMode(int v)  { m_lastMode  = v; }

    // 三种模式状态
    const ZbModeState& trainState() const { return m_train; }
    const ZbModeState& testState() const { return m_test; }

    // 单元列表
    const QList<ZbUnit>& units() const { return m_units; }
    QList<ZbUnit>& units() { return m_units; }

    // 按 LibNo 找单元
    ZbUnit* findUnit(int libNo);

    // 单元是否已完成（速度表非零项 ≥ 题目数）
    static bool isUnitCompleted(const ZbUnit &unit);

private:
    void parseRoot(const QDomElement &root);
    void parseModeState(const QDomElement &elem, ZbModeState &state);
    void parseUnit(const QDomElement &elem, ZbUnit &unit);

    // 用户配置
    QString m_statusName;
    int m_score = 0;
    QString m_date;
    int m_displayType = 0;
    int m_tailTrainMax = 3;
    int m_showSimilarRoot = 0;
    int m_windowsLayout = 0;
    int m_autoTailTrainCount = 0;
    int m_showHitSpeed = 1;

    // 上次单元和模式（0=Train, 1=Test）
    int m_lastLibNo = -1;
    int m_lastMode  = -1;

    // 模式
    ZbModeState m_train;
    ZbModeState m_test;

    // 单元
    QList<ZbUnit> m_units;
};