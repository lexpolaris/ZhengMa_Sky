// StatusBar.cpp
#include "StatusBar.h"
#include <QHBoxLayout>

StatusBar::StatusBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(48);
    setAutoFillBackground(true);

    QFont kaiFont("KaiTi", 16);
    kaiFont.setBold(true);

    m_labelMode = new QLabel("学习方式：", this);
    m_labelUnit = new QLabel("学习单元：", this);
    m_labelClock = new QLabel("00:00:00", this);

    m_labelMode->setFont(kaiFont);
    m_labelUnit->setFont(kaiFont);
    m_labelClock->setFont(kaiFont);

    // 不再 setStyleSheet("color:#006432")，改为用调色板高亮色
    // 通过 QPalette 给这三个 label 设置前景色
    auto applyAccent = [](QLabel *lbl) {
        QPalette p = lbl->palette();
        // 用 Highlight 色作为强调色（深色主题下会自适应）
        p.setColor(QPalette::WindowText, p.color(QPalette::Highlight));
        lbl->setPalette(p);
    };
    applyAccent(m_labelMode);
    applyAccent(m_labelUnit);
    applyAccent(m_labelClock);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->addWidget(m_labelMode);
    layout->addSpacing(40);
    layout->addWidget(m_labelUnit);
    layout->addStretch();
    layout->addWidget(m_labelClock);
}

void StatusBar::setMode(const QString &mode)
{
    m_labelMode->setText("学习方式：" + mode);
}

void StatusBar::setUnitName(const QString &name)
{
    m_labelUnit->setText("学习单元：" + name);
}

void StatusBar::setClock(const QString &time)
{
    m_labelClock->setText(time);
}