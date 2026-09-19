#pragma once
#include <QMainWindow>
#include "UserData.h"
#include "ZmmbTable.h"
#include "UnitSession.h"
#include "SettingsDialog.h"
#include "TestHistory.h"

class StatusBar;
class InfoPanel;
class QuestionPanel;
class TestView;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum class LearnMode { Train, Test };

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onInputSubmitted(const QString &text);
    void onTrainClicked(); //改为切换模式了
    void onSelectLibClicked();
    void onSetupClicked();
    void onStatusClicked();
    void onHelpClicked();
    void onQuitClicked();
    void onLookupChanged(const QString &text);
    void onClockTick();
    void onQuestionChanged();
    void onTestFinished();
    void onHelpRequested();
    void onTestPageFinished();     //  测试页面本页完成
    void onTestItemAnswered(bool correct, int originalIndex);   // 每题提交

private:
    void setupUi();
    void setupConnections();
    bool loadData();
    void loadUnit(int libNo);
    void saveUserData();
    void onTrainRoundsFinished();
    void loadNextUnit();
    void refreshInfoPanel();
    void applyDisplayType(int type);

    void enterTestMode();
    void loadTestPage();
    QList<QPair<QString, QString>> m_testItemsAll;   // 完整题目列表
    QList<int>                     m_testIndicesAll; // 对应的原始索引
    int m_testPageOffset = 0;
    int m_testPageSize = 100;
    int m_testTotalCorrect = 0;
    int m_testTotalWrong = 0;
    qint64 m_testElapsedMs = 0;

    void switchToNextMode();
    void applyLearnMode(LearnMode mode);
    void updateStatusBarMode();

    StatusBar      *m_statusBar = nullptr;
    InfoPanel      *m_infoPanel = nullptr;
    QuestionPanel  *m_questionPanel = nullptr;

    UserData        m_userData;
    ZmmbTable       m_zmmb;
    UnitSession     m_session;

    int             m_currentLibNo = 0;
    QTimer         *m_clockTimer = nullptr;

    LearnMode       m_learnMode = LearnMode::Train;

    // 参数设置
    int m_displayType = 0;          // 0=小写, 1=大写
    int m_hotKey = 121;             // F10
    int m_tailTrainItemsCount = 5;
    bool m_autoTailTrainCount = true;
    int m_tailTrainMaxCount = 3;
    int m_testItemsCount = -1;   // -1 = 自动

    TestHistory m_testHistory;
    void saveTestHistory();

    void finishTest(); 
};