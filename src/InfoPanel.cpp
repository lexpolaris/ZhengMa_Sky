// InfoPanel.cpp
#include "InfoPanel.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFont>

InfoPanel::InfoPanel(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(220);

    setAutoFillBackground(true);   // 背景走 QPalette::Window，边框用 palette 的中色
    // 用 QPalette 给面板画一条右边框（用 Mid 色）
    // 直接 setStyleSheet 带固定色不合适，这里改用一个 QFrame 或自绘。
    // 简单做法：用 palette 里的 Mid 色作为边框，通过 stylesheet 的 palette(...) 语法。
    setStyleSheet(
        "InfoPanel { border-right: 1px solid palette(mid); }");

    QFont songFont("SimSun", 11);

    // 单元名称
    m_labelUnitName = new QLabel("记忆挂钩", this);
    m_labelUnitName->setFont(QFont("SimSun", 13, QFont::Bold));
    m_labelUnitName->setAlignment(Qt::AlignCenter);
    m_labelUnitName->setStyleSheet("color: #006432; padding: 8px;");

    // 信息网格
    auto *infoGrid = new QGridLayout();
    infoGrid->setHorizontalSpacing(8);
    infoGrid->setVerticalSpacing(6);

    auto addRow = [&](int row, const QString &name, QLabel *&valueLabel) {
        auto *nameLabel = new QLabel(name, this);
        nameLabel->setFont(songFont);
        valueLabel = new QLabel("-", this);
        valueLabel->setFont(songFont);
        valueLabel->setStyleSheet("color: #006432;");
        infoGrid->addWidget(nameLabel, row, 0, Qt::AlignLeft);
        infoGrid->addWidget(valueLabel, row, 1, Qt::AlignRight);
    };

    addRow(0, "等级：", m_labelGrade);
    addRow(1, "经验：", m_labelXp);
    addRow(2, "总时间：", m_labelTotalTime);
    addRow(3, "单元时间：", m_labelUnitTime);
    addRow(4, "本次时间：", m_labelSessionTime);
    addRow(5, "字词数：", m_labelCharCount);
    addRow(6, "训练进度：", m_labelProgress);
    addRow(7, "正确率：", m_labelAccuracy);
    addRow(8, "当前速度：", m_labelRecentSpeed);
    addRow(9, "平均/最高：", m_labelSpeed);

    // 功能按钮
    auto *btnLayout = new QVBoxLayout();
    btnLayout->setSpacing(6);

    auto createButton = [&](const QString &text) {
        auto *btn = new QPushButton(text, this);
        btn->setFont(QFont("SimSun", 11));
        btn->setFixedHeight(32);
        // 用 palette(...) 语法，让颜色跟随调色板
        btn->setStyleSheet(
            "QPushButton {"
            "  background-color: palette(button);"
            "  color: palette(button-text);"
            "  border: 1px solid palette(mid);"
            "  border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "  background-color: palette(highlight);"
            "}"
            "QPushButton:pressed {"
            "  background-color: palette(mid);"
            "}");
        return btn;
    };

    auto *btnSwitchModeClicked = createButton("切换模式");
    auto *btnSelectLib = createButton("选择单元库");
    auto *btnSetup = createButton("参数设置");
    auto *btnStatus = createButton("测试记录");
    auto *btnHelp = createButton("帮助");
    auto *btnQuit = createButton("退出");

    connect(btnSwitchModeClicked, &QPushButton::clicked, this, &InfoPanel::switchModeClicked);
    connect(btnSelectLib, &QPushButton::clicked, this, &InfoPanel::selectLibClicked);
    connect(btnSetup, &QPushButton::clicked, this, &InfoPanel::setupClicked);
    connect(btnStatus, &QPushButton::clicked, this, &InfoPanel::statusClicked);
    connect(btnHelp, &QPushButton::clicked, this, &InfoPanel::helpClicked);
    connect(btnQuit, &QPushButton::clicked, this, &InfoPanel::quitClicked);

    btnLayout->addWidget(btnSwitchModeClicked);
    btnLayout->addWidget(btnSelectLib);
    btnLayout->addWidget(btnSetup);
    btnLayout->addWidget(btnStatus);
    btnLayout->addWidget(btnHelp);
    btnLayout->addWidget(btnQuit);

    // 编码查询
    auto *lookupLabel = new QLabel("编码查询：", this);
    lookupLabel->setFont(songFont);
    m_editLookup = new QLineEdit(this);
    m_editLookup->setFont(QFont("SimSun", 11));
    m_editLookup->setFixedHeight(28);
    connect(m_editLookup, &QLineEdit::textChanged, this, &InfoPanel::lookupChanged);

    // 编码查询结果
    m_labelLookupResult = new QLabel("", this);
    m_labelLookupResult->setFont(QFont("SimSun", 10));
    m_labelLookupResult->setWordWrap(true);
    m_labelLookupResult->setStyleSheet("color: #006432; padding: 2px;");
    m_labelLookupResult->setMinimumHeight(24);

    // 整体布局
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    mainLayout->addWidget(m_labelUnitName);
    mainLayout->addLayout(infoGrid);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(lookupLabel);
    mainLayout->addWidget(m_editLookup);
    mainLayout->addWidget(m_labelLookupResult);
}

void InfoPanel::setUnitName(const QString &name) { m_labelUnitName->setText(name); }

void InfoPanel::setGrade(int grade) { m_labelGrade->setText(QString::number(grade)); }

void InfoPanel::setTotalTime(const QString &t) { m_labelTotalTime->setText(t); }

void InfoPanel::setUnitTime(const QString &t) { m_labelUnitTime->setText(t); }

void InfoPanel::setSessionTime(const QString &t) { m_labelSessionTime->setText(t); }

void InfoPanel::setCharCount(int c) { m_labelCharCount->setText(QString::number(c)); }

void InfoPanel::setProgress(int roundLeft, int roundTotal,
                            int roundsLeft, int roundsTotal,
                            int roundsDone)
{
    m_labelProgress->setText(
        QString("%1/%2  %3/%4  %5")
            .arg(roundLeft)
            .arg(roundTotal)
            .arg(roundsLeft)
            .arg(roundsTotal)
            .arg(roundsDone));
}

void InfoPanel::setAccuracy(int p) { m_labelAccuracy->setText(QString("%1%").arg(p)); }

void InfoPanel::setSpeed(int cur, int best)
{
    m_labelSpeed->setText(QString("%1/%2").arg(cur).arg(best));
}

void InfoPanel::setRecentSpeed(int recent)
{
    m_labelRecentSpeed->setText(QString::number(recent));
}

void InfoPanel::setXp(long long xp, long long next, long long atLevel)
{
    Q_UNUSED(atLevel);
    // 显示累计总经验，并提示距下一级还差多少：如 "1234（还差566）"
    //   xp   = 累计总经验（只增不减）
    //   next = 升到下一级的累计门槛
    if (next > xp) {
        const long long remain = next - xp;
        m_labelXp->setText(QString("%1（还差%2）").arg(xp).arg(remain));
    } else {
        m_labelXp->setText(QString::number(xp));
    }
}

void InfoPanel::setLookupResult(const QString &text)
{
    m_labelLookupResult->setText(text);
}
