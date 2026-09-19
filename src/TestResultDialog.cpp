// TestResultDialog.cpp
#include "TestResultDialog.h"
#include "SpeedTracker.h"

#include <QTabWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QFont>
#include <QClipboard>
#include <QApplication>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>

TestResultDialog::TestResultDialog(const TestHistory &history,
                                   int currentLibNo,
                                   QWidget *parent)
    : QDialog(parent), m_history(history), m_libNo(currentLibNo)
{
    setWindowTitle("测试记录");
    resize(800, 560);
    setupUi();
}

void TestResultDialog::setupUi()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setFont(QFont("SimSun", 11));

    setupChartTab();
    setupHistoryTab();

    auto *btnClose = new QPushButton("关闭", this);
    btnClose->setFixedHeight(32);
    btnClose->setMinimumWidth(80);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(btnClose);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(m_tabWidget, 1);
    layout->addLayout(btnLayout);
}

void TestResultDialog::setupChartTab()
{
    auto *tab = new QWidget(m_tabWidget);

    m_chartView = new QChartView(tab);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    m_checkShowHitSpeed = new QCheckBox("显示击键速度曲线", tab);
    m_checkShowHitSpeed->setFont(QFont("SimSun", 11));
    m_checkShowHitSpeed->setChecked(true);
    connect(m_checkShowHitSpeed, &QCheckBox::toggled,
            this, &TestResultDialog::refreshChart);

    m_btnClearHistory = new QPushButton("清除历史记录", tab);
    m_btnClearHistory->setFixedHeight(28);
    connect(m_btnClearHistory, &QPushButton::clicked,
            this, &TestResultDialog::onClearHistory);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(m_checkShowHitSpeed);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_btnClearHistory);

    auto *layout = new QVBoxLayout(tab);
    layout->addWidget(m_chartView, 1);
    layout->addLayout(bottomLayout);

    m_tabWidget->addTab(tab, "测试历史曲线");
    refreshChart();
}

void TestResultDialog::refreshChart()
{
    const auto records = m_history.recordsByUnit(m_libNo);

    auto *chart = new QChart();
    chart->setTitle(QString("《郑码天空》：平均速度 / 测试成绩"));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // 速度曲线
    auto *speedSeries = new QLineSeries();
    speedSeries->setName("速度");

    // 成绩曲线（速度 - 错误数）
    auto *scoreSeries = new QLineSeries();
    scoreSeries->setName("成绩");

    // 击键速度曲线（可选）
    auto *hitSpeedSeries = new QLineSeries();
    hitSpeedSeries->setName("击键速度");

    int x = 0;
    int maxY = 0;
    for (const TestRecord &r : records) {
        speedSeries->append(x, r.speed);
        int score = r.speed - r.wrongCount;
        scoreSeries->append(x, score);
        hitSpeedSeries->append(x, r.bestSpeed);

        maxY = std::max({maxY, r.speed, score, r.bestSpeed});
        ++x;
    }

    chart->addSeries(speedSeries);
    chart->addSeries(scoreSeries);
    if (m_checkShowHitSpeed->isChecked())
        chart->addSeries(hitSpeedSeries);

    // 坐标轴
    auto *axisX = new QValueAxis();
    axisX->setTitleText("测试次数");
    axisX->setLabelFormat("%d");
    axisX->setRange(0, std::max(1, x - 1));

    auto *axisY = new QValueAxis();
    axisY->setTitleText("速度 / 成绩");
    axisY->setRange(0, maxY + 20);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    for (auto *s : chart->series()) {
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }

    m_chartView->setChart(chart);
}

void TestResultDialog::setupHistoryTab()
{
    auto *tab = new QWidget(m_tabWidget);

    auto *title = new QLabel("《郑码天空》：测试历史记录", tab);
    title->setAlignment(Qt::AlignCenter);
    title->setFont(QFont("KaiTi", 14, QFont::Bold));
    title->setStyleSheet("color: #006432; padding: 6px;");

    m_textHistory = new QTextEdit(tab);
    m_textHistory->setReadOnly(true);
    m_textHistory->setFont(QFont("SimSun", 11));
    m_textHistory->setStyleSheet("background-color: #F8F8F8;");

    m_btnCopyHistory = new QPushButton("复制测试历史记录", tab);
    m_btnCopyHistory->setFixedHeight(28);
    connect(m_btnCopyHistory, &QPushButton::clicked,
            this, &TestResultDialog::onCopyHistory);

    auto *layout = new QVBoxLayout(tab);
    layout->addWidget(title);
    layout->addWidget(m_textHistory, 1);
    layout->addWidget(m_btnCopyHistory, 0, Qt::AlignCenter);

    m_tabWidget->addTab(tab, "测试历史记录");
    refreshHistory();
}

void TestResultDialog::refreshHistory()
{
    const auto records = m_history.recordsByUnit(m_libNo);

    QString html;
    html += "<table border='0' cellpadding='4' style='font-family:SimSun;font-size:11pt;'>";
    html += "<tr style='background-color:#E0E0E0;'>"
            "<th>序号</th><th>日期</th><th>速度</th>"
            "<th>成绩</th><th>正确率</th><th>错误数</th><th>用时</th></tr>";

    int i = 1;
    for (const TestRecord &r : records) {
        int score = r.speed - r.wrongCount;
        html += QString("<tr>"
                        "<td align='center'>%1</td>"
                        "<td align='center'>%2</td>"
                        "<td align='center'>%3</td>"
                        "<td align='center'>%4</td>"
                        "<td align='center'>%5%</td>"
                        "<td align='center'>%6</td>"
                        "<td align='center'>%7</td>"
                        "</tr>")
                .arg(i++)
                .arg(r.dateTime.toString("yyyy-MM-dd HH:mm"))
                .arg(r.speed)
                .arg(score)
                .arg(r.accuracy)
                .arg(r.wrongCount)
                .arg(SpeedTracker::formatDuration(r.totalMs));
    }
    html += "</table>";

    if (records.isEmpty())
        html = "<p style='color:#888;'>暂无测试记录</p>";

    m_textHistory->setHtml(html);
}

void TestResultDialog::onClearHistory()
{
    if (QMessageBox::question(this, "确认", "确定清除该单元的所有测试记录？")
        == QMessageBox::Yes)
    {
        // 注意：这里只清除内存中的记录，实际需要通知 MainWindow 持久化
        // 简化做法：直接清空 m_history（const_cast）
        auto &mutableHistory = const_cast<TestHistory &>(m_history);
        mutableHistory.clear();
        refreshChart();
        refreshHistory();
    }
}

void TestResultDialog::onCopyHistory()
{
    QApplication::clipboard()->setText(m_textHistory->toPlainText());
    QMessageBox::information(this, "提示", "已复制到剪贴板");
}