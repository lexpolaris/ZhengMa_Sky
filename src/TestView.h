#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QElapsedTimer>
#include <QColor>

class TestView : public QWidget
{
    Q_OBJECT
public:
    explicit TestView(QWidget *parent = nullptr);

    // 加载当前页题目
    // items: (显示文本, 正确编码, 原始索引)
    void loadItems(const QList<QPair<QString, QString>> &items,
                   const QList<int> &originalIndices);

    // 提交输入
    bool submitInput(const QString &input);

    // 状态
    int currentIndex() const { return m_currentIndex; }
    int totalCount() const { return m_items.size(); }
    int correctCount() const;   // 本页首次答对的题数
    int wrongCount() const;     // 本页累积错误次数（含重试）
    bool isFinished() const { return m_currentIndex >= m_items.size(); }

    // 本页能放多少题（供 MainWindow 分页）
    int pageCapacity() const;

    // 全局信息（页码、累计正确/错误）
    void setPageInfo(const QString &info);
    void setGlobalCounters(int correct, int wrong);

    // 重置
    void reset();

    // 计时
    qint64 elapsedMs() const {
        if (m_finalElapsedMs >= 0) return m_finalElapsedMs;
        return m_timer.isValid() ? m_timer.elapsed() : 0;
    }
    void startTimer() { m_timer.start(); m_finalElapsedMs = -1; }
    void stopTimer() { if (m_timer.isValid()) m_finalElapsedMs = m_timer.elapsed(); }

signals:
    //  每次提交都发：correct=true 表示本题通过，false 表示答错
    //  originalIndex 是该题在完整测试列表里的原始索引
    void itemAnswered(bool correct, int originalIndex);
    // 本页打完
    void finished();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    struct TestItem {
        QString text;
        QString code;
        int originalIndex = -1;    // 在完整列表里的索引
        enum State { Pending, Correct, Wrong } state = Pending;
        bool passedOnce = false;   // 是否已经答对过一次（用于 correctCount 不重复计）
    };

    void setupUi();
    void refreshDisplay();
    void updateProgress();
    QColor colorForState(TestItem::State state, bool isCurrent) const;

    QList<TestItem> m_items;
    int m_currentIndex = 0;

    QTextEdit *m_textDisplay = nullptr;
    QLabel    *m_labelProgress = nullptr;
    QLabel    *m_labelPageInfo = nullptr;
    QLabel    *m_labelHint = nullptr;
    QLineEdit *m_editInput = nullptr;

    QElapsedTimer m_timer;
    qint64 m_finalElapsedMs = -1;

    int m_currentWrongStreak = 0;
    int m_totalWrong = 0;         // 本页累积错误次数（含重试）

    // 全局累计（由 MainWindow 设置）
    int m_globalCorrect = 0;
    int m_globalWrong = 0;

    static constexpr int kWrongStreakForHint = 3;
    static constexpr int kCharsPerLine = 15;
    static constexpr double kLineHeightFactor = 1.5;
};