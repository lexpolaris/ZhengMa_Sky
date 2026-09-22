#include "TestView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QPalette>
#include <QDebug>

TestView::TestView(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void TestView::setupUi()
{
    setAutoFillBackground(true);

    // 内容显示区
    m_textDisplay = new QTextEdit(this);
    m_textDisplay->setReadOnly(true);
    m_textDisplay->setFont(QFont("zmzg", 26));
    m_textDisplay->setFrameStyle(QFrame::NoFrame);
    m_textDisplay->setStyleSheet(
        "QTextEdit {"
        "  border: none;"
        "  background-color: palette(base);"
        "  color: palette(text);"
        "  padding: 0px;"
        "}");
    m_textDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 进度
    m_labelProgress = new QLabel("0/0", this);
    m_labelProgress->setAlignment(Qt::AlignCenter);
    m_labelProgress->setFont(QFont("SimSun", 12));
    {
        QPalette p = m_labelProgress->palette();
        p.setColor(QPalette::WindowText, p.color(QPalette::Mid));
        m_labelProgress->setPalette(p);
    }

    // 页码
    m_labelPageInfo = new QLabel("", this);
    m_labelPageInfo->setAlignment(Qt::AlignCenter);
    m_labelPageInfo->setFont(QFont("SimSun", 11));
    {
        QPalette p = m_labelPageInfo->palette();
        p.setColor(QPalette::WindowText, p.color(QPalette::Mid));
        m_labelPageInfo->setPalette(p);
    }

    // 提示
    m_labelHint = new QLabel("", this);
    m_labelHint->setAlignment(Qt::AlignCenter);
    m_labelHint->setFont(QFont("SimSun", 13));
    m_labelHint->setWordWrap(true);
    m_labelHint->setMinimumHeight(28);
    {
        QPalette p = m_labelHint->palette();
        p.setColor(QPalette::WindowText, p.color(QPalette::Link));
        m_labelHint->setPalette(p);
    }

    // 输入框
    m_editInput = new QLineEdit(this);
    m_editInput->setAlignment(Qt::AlignCenter);
    m_editInput->setFont(QFont("Arial", 24));
    m_editInput->setFixedHeight(48);
    m_editInput->setStyleSheet(
        "QLineEdit {"
        "  border: 2px solid palette(mid);"
        "  border-radius: 6px;"
        "  background-color: palette(base);"
        "  color: palette(text);"
        "  padding: 4px;"
        "}"
        "QLineEdit:focus {"
        "  border-color: palette(highlight);"
        "}");
    m_editInput->installEventFilter(this);

    connect(m_editInput, &QLineEdit::returnPressed, this, [this]() {
        submitInput(m_editInput->text());
    });

    // 布局
    auto *progressLayout = new QHBoxLayout();
    progressLayout->addStretch();
    progressLayout->addWidget(m_labelProgress);
    progressLayout->addWidget(m_labelPageInfo);
    progressLayout->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);
    layout->addWidget(m_textDisplay, 1);
    layout->addLayout(progressLayout);
    layout->addWidget(m_labelHint);
    layout->addWidget(m_editInput);
}

QColor TestView::colorForState(TestItem::State state, bool isCurrent) const
{
    const QPalette pal = palette();

    switch (state) {
    case TestItem::Pending:
        return isCurrent ? pal.color(QPalette::Highlight)
                         : pal.color(QPalette::Text);
    case TestItem::Correct:
        return pal.color(QPalette::Link);
    case TestItem::Wrong:
        {
            QColor c = pal.color(QPalette::BrightText);
            if (c == pal.color(QPalette::Text))
                c = QColor(0xCC, 0x00, 0x00);
            return c;
        }
    }
    return pal.color(QPalette::Text);
}

int TestView::pageCapacity() const
{
    if (!m_textDisplay) return 100;

    const QFontMetrics fm(m_textDisplay->font());
    const int lineH = qMax(1, static_cast<int>(fm.lineSpacing() * kLineHeightFactor));
    const int viewportH = m_textDisplay->viewport()->height() - 48;
    if (viewportH < 50) return 100;   // 布局未完成，返回默认值

    const int lines = qMax(1, viewportH / lineH - 1);
    return lines * kCharsPerLine;
}

void TestView::setPageInfo(const QString &info)
{
    m_labelPageInfo->setText(info);
}

void TestView::setGlobalCounters(int correct, int wrong)
{
    m_globalCorrect = correct;
    m_globalWrong = wrong;
    updateProgress();
}

void TestView::loadItems(const QList<QPair<QString, QString>> &items,
                         const QList<int> &originalIndices)
{
    m_items.clear();
    m_currentIndex = 0;
    m_totalWrong = 0;
    m_currentWrongStreak = 0;
    m_keyStrokes = 0;
    m_firstRoundCorrect = 0;
    m_retesting = false;
    m_retestRound = 0;

    for (int i = 0; i < items.size(); ++i) {
        TestItem item;
        item.text = items[i].first;
        item.code = items[i].second;
        item.originalIndex = (i < originalIndices.size()) ? originalIndices[i] : i;
        item.state = TestItem::Pending;
        item.passedOnce = false;
        item.everWrong = false;
        m_items.append(item);
    }

    m_editInput->clear();
    m_editInput->setFocus();
    if (m_labelHint) m_labelHint->clear();

    refreshDisplay();
    startTimer();
}

void TestView::reset()
{
    for (auto &item : m_items) {
        item.state = TestItem::Pending;
        item.passedOnce = false;
        item.everWrong = false;
    }
    m_currentIndex = 0;
    m_totalWrong = 0;
    m_currentWrongStreak = 0;
    m_keyStrokes = 0;
    m_firstRoundCorrect = 0;
    m_retesting = false;
    m_retestRound = 0;
    m_editInput->clear();
    if (m_labelHint) m_labelHint->clear();
    refreshDisplay();
    startTimer();
}

void TestView::refreshDisplay()
{
    if (m_items.isEmpty()) {
        m_textDisplay->clear();
        updateProgress();
        return;
    }

    QString html;
    html += QString("<div style='line-height:%1;'>").arg(kLineHeightFactor);

    for (int i = 0; i < m_items.size(); ++i) {
        const TestItem &item = m_items[i];

        QColor color = colorForState(item.state, i == m_currentIndex);
        const QString text = item.text;

        QString style = QString("color:%1;").arg(color.name());
        if (i == m_currentIndex)
            style += "text-decoration:underline;";

        html += QString("<span style='%1'>%2</span>")
                    .arg(style, text.toHtmlEscaped());

        if ((i + 1) % kCharsPerLine == 0 && (i + 1) < m_items.size())
            html += "<br>";
    }

    html += "</div>";
    m_textDisplay->setHtml(html);

    updateProgress();
    m_editInput->setFocus();
}

void TestView::updateProgress()
{
    // 本页进度：当前题 / 本页总题
    m_labelProgress->setText(
        QString("进度：%1/%2    正确：%3    错误：%4")
            .arg(m_currentIndex)
            .arg(m_items.size())
            .arg(m_globalCorrect + correctCount())
            .arg(m_globalWrong + m_totalWrong));
}

int TestView::correctCount() const
{
    // 首轮：实时统计首轮首次答对数；进入重测后 items 只剩错题，用保存值
    if (m_retesting)
        return m_firstRoundCorrect;

    int n = 0;
    for (const auto &item : m_items)
        if (item.passedOnce) ++n;
    return n;
}

int TestView::wrongCount() const
{
    return m_totalWrong;
}

bool TestView::submitInput(const QString &input)
{
    if (m_currentIndex >= m_items.size()) return false;
    if (input.isEmpty()) return false;

    // 累计击键次数（本次输入的编码字符数）
    m_keyStrokes += qMax(1, input.trimmed().length());

    TestItem &item = m_items[m_currentIndex];

    // 判定
    QStringList codes = item.code.split(' ', Qt::SkipEmptyParts);
    bool correct = false;
    for (const QString &code : codes) {
        if (code.compare(input.trimmed(), Qt::CaseInsensitive) == 0) {
            correct = true;
            break;
        }
    }

    const int answeredIndex = m_currentIndex;
    const int originalIdx = item.originalIndex;

    if (correct) {
        // 无论是否第一次答对，都标记为 Correct
        item.state = TestItem::Correct;
        // 仅首轮记录「首次答对」，用于 correctCount；重测轮不重复计
        if (!m_retesting)
            item.passedOnce = true;

        ++m_currentIndex;
        m_editInput->clear();
        m_currentWrongStreak = 0;
        if (m_labelHint) m_labelHint->clear();

        // 通知 MainWindow：本题正确
        emit itemAnswered(true, originalIdx);

        if (m_currentIndex >= m_items.size()) {
            // 本轮跑完：若还有曾答错的题，则进入错题重测
            if (startRetest())
                return true;

            refreshDisplay();
            stopTimer();
            emit finished();      // 本页全部通过
            return true;
        }    } else {
        // 每次答错都累积 +1
        item.state = TestItem::Wrong;
        item.everWrong = true;
        ++m_totalWrong;
        m_editInput->clear();
        ++m_currentWrongStreak;

        if (m_currentWrongStreak >= kWrongStreakForHint) {
            const QString hint = QString("提示：%1 = %2")
                                    .arg(item.text, item.code);
            m_labelHint->setText(hint);
        }

        // 通知 MainWindow：本题答错（用于回写 speedTable）
        emit itemAnswered(false, originalIdx);
    }

    refreshDisplay();
    return correct;
}

bool TestView::startRetest()
{
    // 首次进入重测前，保存首轮首次答对数
    if (!m_retesting) {
        int n = 0;
        for (const auto &item : m_items)
            if (item.passedOnce) ++n;
        m_firstRoundCorrect = n;
    }

    // 收集本轮曾答错的题（保持原顺序）
    QList<TestItem> wrongItems;
    for (const auto &item : m_items) {
        if (item.everWrong) {
            TestItem t = item;
            t.state = TestItem::Pending;
            t.passedOnce = false;
            t.everWrong = false;   // 新一轮重新统计
            wrongItems.append(t);
        }
    }

    if (wrongItems.isEmpty())
        return false;              // 无错题，测试结束

    // 进入（或继续）错题重测：items 仅保留错题
    m_items = wrongItems;
    m_currentIndex = 0;
    m_currentWrongStreak = 0;
    m_retesting = true;
    ++m_retestRound;
    m_editInput->clear();
    if (m_labelHint) {
        m_labelHint->setText(
            QString("错题重测（第 %1 轮）：共 %2 题")
                .arg(m_retestRound).arg(m_items.size()));
    }

    refreshDisplay();
    return true;
}

void TestView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateProgress();
}

bool TestView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_editInput && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Space) {
            submitInput(m_editInput->text());
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}