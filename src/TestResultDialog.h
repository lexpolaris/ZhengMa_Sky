// TestResultDialog.h
#pragma once
#include <QDialog>
#include "TestHistory.h"

#include <QtCharts/QChartView>   // 包含头文件即可

class QTabWidget;
class QTextEdit;
class QPushButton;
class QCheckBox;

class TestResultDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TestResultDialog(const TestHistory &history,
                              int currentLibNo,
                              QWidget *parent = nullptr);

private slots:
    void onClearHistory();
    void onCopyHistory();

private:
    void setupUi();
    void setupChartTab();
    void setupHistoryTab();
    void refreshChart();
    void refreshHistory();

    const TestHistory &m_history;
    int m_libNo;

    QTabWidget *m_tabWidget = nullptr;

    // 曲线页 —— 去掉 QtCharts:: 前缀
    QChartView  *m_chartView = nullptr;
    QCheckBox   *m_checkShowHitSpeed = nullptr;
    QPushButton *m_btnClearHistory = nullptr;

    // 记录页
    QTextEdit   *m_textHistory = nullptr;
    QPushButton *m_btnCopyHistory = nullptr;
};