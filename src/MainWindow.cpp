#include "MainWindow.h"
#include "StatusBar.h"
#include "InfoPanel.h"
#include "QuestionPanel.h"
#include "UnitSelectDialog.h"
#include "SettingsDialog.h"
#include "TestResultDialog.h"
#include "HelpDialog.h"
#include "TestView.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QTime>
#include <QMessageBox>
#include <QDir>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("郑码天空");
    resize(1000, 680);

    setupUi();
    applyDisplayType(m_userData.displayType());
    setupConnections();

    if (!loadData()) {
        QMessageBox::warning(this, "错误", "数据文件加载失败");
    }

    // 恢复上次单元与模式
    const int lastLib = m_userData.lastLibNo();
    if (lastLib > 0 && m_userData.findUnit(lastLib)) {
        m_currentLibNo = lastLib;
    }

    // 如果上次单元已完成，跳到下一个未完成单元
    {
        const auto &units = m_userData.units();
        ZbUnit *cur = m_userData.findUnit(m_currentLibNo);
        if (cur && UserData::isUnitCompleted(*cur)) {
            const auto &units = m_userData.units();
            int startIdx = 0;
            for (int i = 0; i < units.size(); ++i) {
                if (units[i].libNo == m_currentLibNo) {
                    startIdx = i;
                    break;
                }
            }

            // 从当前单元之后开始找第一个未完成的
            for (int k = 1; k <= units.size(); ++k) {
                const int idx = (startIdx + k) % units.size();
                if (!UserData::isUnitCompleted(units[idx])) {
                    m_currentLibNo = units[idx].libNo;
                    break;
                }
            }
            // 如果全完成，保持原单元
        }
    }

    const int lastMode = m_userData.lastMode();
    if (lastMode >= 0 && lastMode <= 2)
        m_learnMode = static_cast<LearnMode>(lastMode);
    else
        m_learnMode = LearnMode::Train;

    loadUnit(m_currentLibNo);

    // 如果上次不是 Train，延迟切到对应模式（等窗口显示后）
    if (m_learnMode != LearnMode::Train) {
        const LearnMode target = m_learnMode;
        QTimer::singleShot(0, this, [this, target]() {
            applyLearnMode(target);
        });
    } else {
        updateStatusBarMode();
    }

    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::onClockTick);
    m_clockTimer->start(500);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_statusBar = new StatusBar(central);
    m_statusBar->setUnitName("笔画");

    auto *middleWidget = new QWidget(central);
    auto *middleLayout = new QHBoxLayout(middleWidget);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(0);

    m_infoPanel = new InfoPanel(middleWidget);
    m_questionPanel = new QuestionPanel(middleWidget);

    middleLayout->addWidget(m_infoPanel);
    middleLayout->addWidget(m_questionPanel, 1);

    mainLayout->addWidget(m_statusBar);
    mainLayout->addWidget(middleWidget, 1);

    setCentralWidget(central);
}

void MainWindow::setupConnections()
{
    connect(m_questionPanel, &QuestionPanel::inputSubmitted,
            this, &MainWindow::onInputSubmitted);

    connect(m_infoPanel, &InfoPanel::switchModeClicked,
            this, &MainWindow::onTrainClicked);
    connect(m_infoPanel, &InfoPanel::selectLibClicked,
            this, &MainWindow::onSelectLibClicked);
    connect(m_infoPanel, &InfoPanel::setupClicked,
            this, &MainWindow::onSetupClicked);
    connect(m_infoPanel, &InfoPanel::statusClicked,
            this, &MainWindow::onStatusClicked);
    connect(m_infoPanel, &InfoPanel::helpClicked,
            this, &MainWindow::onHelpClicked);
    connect(m_infoPanel, &InfoPanel::quitClicked,
            this, &MainWindow::onQuitClicked);
    connect(m_infoPanel, &InfoPanel::lookupChanged,
            this, &MainWindow::onLookupChanged);

    connect(&m_session, &UnitSession::questionChanged,
            this, &MainWindow::onQuestionChanged);
        
    connect(m_questionPanel, &QuestionPanel::helpRequested,
        this, &MainWindow::onHelpRequested);

    TestView *tv = m_questionPanel->testView();
    connect(tv, &TestView::finished, this, &MainWindow::onTestPageFinished);
    connect(tv, &TestView::itemAnswered, this, &MainWindow::onTestItemAnswered);
}

bool MainWindow::loadData()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString trainPath = QDir(appDir).filePath("data/Train.xml");
    const QString userPath  = QDir(appDir).filePath("data/user.xml");
    const QString zmmbPath  = QDir(appDir).filePath("data/zmmb.txt");
    const QString historyPath = QDir(appDir).filePath("data/test_history.xml");

    // 1. 加载模板
    if (!m_userData.loadTemplate(trainPath)) {
        qWarning() << "模板加载失败";
        return false;
    }

    // 2. 加载用户数据（覆盖模板进度）
    if (!m_userData.loadUserData(userPath)) {
        qDebug() << "用户数据不存在，将使用模板初始化";
        // 不做任何事，第一次启动就是模板状态
    }

    // 3. 加载码表
    if (!m_zmmb.load(zmmbPath)) return false;

    // 4. 加载测试记录
    m_testHistory.load(historyPath);

    qDebug() << "数据加载完成:"
             << m_userData.units().size() << "个单元,"
             << m_zmmb.size() << "条码表";
    return true;
}

void MainWindow::saveUserData()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString path = QDir(appDir).filePath("data/user.xml");
    m_userData.saveUserData(path);
}

void MainWindow::loadUnit(int libNo)
{
    ZbUnit *unit = m_userData.findUnit(libNo);
    if (!unit) return;

    m_currentLibNo = libNo;

    // 记录到 UserData
    m_userData.setLastLibNo(libNo);
    m_userData.setLastMode(static_cast<int>(m_learnMode));

    // 使用用户配置的 TailTrainMax
    const int trainMax = m_autoTailTrainCount
                        ? m_userData.tailTrainMax()   // 自动：从 user0.xml 读
                        : m_tailTrainMaxCount;        // 手动：用户设置

    m_session.setTestItemsOverride(m_testItemsCount);
    m_session.start(unit, &m_zmmb, trainMax);

    refreshInfoPanel();
    m_questionPanel->focusInput();

    saveUserData();
}

void MainWindow::refreshInfoPanel()
{
    const ZbUnit *unit = m_userData.findUnit(m_currentLibNo);
    if (!unit) return;

    m_infoPanel->setUnitName(unit->libName);
    m_infoPanel->setGrade(m_session.speed().grade());
    m_infoPanel->setTotalTime(
        SpeedTracker::formatDuration(m_session.speed().totalMs()));
    m_infoPanel->setUnitTime(
        SpeedTracker::formatDuration(m_session.speed().unitMs()));
    m_infoPanel->setSessionTime(
        SpeedTracker::formatDuration(m_session.speed().sessionMs()));
    m_infoPanel->setCharCount(m_session.speed().charCount());

    // 训练进度：本轮剩余题数 / 本轮总题数
    //            剩余轮数 / 总轮数
    //            已完成轮数
    const int roundTotal = m_session.roundCount();
    const int curIdx = m_session.currentIndex();       // 0-based 当前题下标
    const int roundLeft = qMax(0, roundTotal - curIdx - 1);

    const int roundsTotal = m_session.totalRounds();
    const int roundsDone  = m_session.currentRound();
    const int roundsLeft  = qMax(0, roundsTotal - roundsDone);

    m_infoPanel->setProgress(roundLeft, roundTotal,
                             roundsLeft, roundsTotal,
                             roundsDone);

    m_infoPanel->setAccuracy(m_session.speed().accuracy());
    m_infoPanel->setSpeed(m_session.speed().currentSpeed(),
                          m_session.speed().bestSpeed());

    m_statusBar->setUnitName(unit->libName);
}


void MainWindow::onQuestionChanged()
{
    // qDebug() << ">>> onQuestionChanged 被调用";

    const ZbItem *item = m_session.currentItem();
    if (!item) {
        // qWarning() << ">>> currentItem 为空！";
        return;
    }
    // qDebug() << ">>> 显示题目:" << item->charText
    //          << " 联想:" << item->associate;

    m_questionPanel->setQuestion(item->charText);
    m_questionPanel->setAssociate(item->associate);
    m_questionPanel->setHint(item->split);
    m_questionPanel->clearInput();
    m_questionPanel->focusInput();

    refreshInfoPanel();
}

void MainWindow::onInputSubmitted(const QString &text)
{
    if (text.isEmpty()) return;

    const ZbItem *item = m_session.currentItem();
    if (!item) return;

    const bool correct = m_session.submit(text);

    if (correct) {
        // 答对：显示正确，进入下一题
        m_questionPanel->setAssociate("✓ 正确", "#006432");

        // 判断轮次
        if (m_session.isRoundEnd()) {
            if (m_session.isTestMode()) {
                finishTest();
            } else if (m_session.isAllRoundsEnd()) {
                onTrainRoundsFinished();
                return;      // 注意：这里 return，避免后面 clearInput/focusInput 重复
            } else {
                m_session.nextRound();
            }
        } else {
            m_session.next();
        }

        m_questionPanel->clearInput();
        m_questionPanel->focusInput();
        refreshInfoPanel();

    } else {
        // 答错：显示错误，不进入下一题，等待重试
        QStringList codes;
        if (!item->code.isEmpty())
            codes << item->code;
        else
            codes = m_zmmb.lookup(item->charText);

        m_questionPanel->setAssociate(
            "✗ 错误。正确编码：" + codes.join(' '),
            "#CC0000");

        // 输入框清空，方便重试
        m_questionPanel->clearInput();
        m_questionPanel->focusInput();

        // 刷新统计（错误数变了）
        refreshInfoPanel();
    }
}

void MainWindow::onTrainRoundsFinished()
{
    ZbUnit *unit = m_userData.findUnit(m_currentLibNo);
    const QString unitName = unit ? unit->libName : QString();

    // 刷新 Used
    if (unit) {
        unit->used = UserData::isUnitCompleted(*unit) ? 1 : 0;
    }
    saveUserData();   // 立即持久化

    const bool completed = unit && unit->used == 1;

    QMessageBox box(this);
    box.setWindowTitle("训练结束");
    box.setIcon(QMessageBox::Question);
    box.setText(completed
        ? QString("《%1》已完成！\n请选择下一步：").arg(unitName)
        : QString("《%1》训练结束。\n请选择下一步：").arg(unitName));

    QPushButton *btnTest    = box.addButton("进入测试", QMessageBox::AcceptRole);
    QPushButton *btnNext    = box.addButton("下一单元", QMessageBox::ActionRole);
    QPushButton *btnAgain   = box.addButton("继续训练", QMessageBox::ActionRole);
    QPushButton *btnCancel  = box.addButton("关闭",     QMessageBox::RejectRole);

    box.setDefaultButton(btnTest);
    box.exec();

    QAbstractButton *clicked = box.clickedButton();

    if (clicked == btnTest) {
        // 进入测试
        m_learnMode = LearnMode::Test;
        m_userData.setLastMode(static_cast<int>(m_learnMode));
        applyLearnMode(LearnMode::Test);
    } else if (clicked == btnNext) {
        // 下一单元
        loadNextUnit();
    } else if (clicked == btnAgain) {
        // 重做当前单元
        loadUnit(m_currentLibNo);
        updateStatusBarMode();
    } else {
        // 关闭：停在当前题目上
        m_questionPanel->focusInput();
    }
}

void MainWindow::loadNextUnit()
{
    const auto &units = m_userData.units();
    if (units.isEmpty()) return;

    // 找当前单元的位置
    int curIdx = 0;
    for (int i = 0; i < units.size(); ++i) {
        if (units[i].libNo == m_currentLibNo) {
            curIdx = i;
            break;
        }
    }

    // 从下一个开始，找第一个未完成的单元
    int nextLibNo = units[(curIdx + 1) % units.size()].libNo;
    for (int k = 1; k <= units.size(); ++k) {
        const int idx = (curIdx + k) % units.size();
        if (!UserData::isUnitCompleted(units[idx])) {
            nextLibNo = units[idx].libNo;
            break;
        }
    }

    m_learnMode = LearnMode::Train;
    m_userData.setLastMode(static_cast<int>(m_learnMode));

    loadUnit(nextLibNo);
    updateStatusBarMode();
}

void MainWindow::finishTest()
{
    // 保存测试记录
    TestRecord rec;
    rec.libNo      = m_currentLibNo;
    ZbUnit *unit = m_userData.findUnit(m_currentLibNo);
    rec.libName    = unit ? unit->libName : "";
    rec.speed      = m_session.speed().currentSpeed();
    rec.bestSpeed  = m_session.speed().bestSpeed();
    rec.hitSpeed   = m_session.speed().hitSpeed();
    rec.accuracy   = m_session.speed().accuracy();
    rec.wrongCount = m_session.speed().wrongCount();
    rec.totalMs    = m_session.speed().sessionMs();
    rec.dateTime   = QDateTime::currentDateTime();

    m_testHistory.addRecord(rec);
    saveTestHistory();

    QMessageBox::information(this, "测试结束",
        QString("平均速度：%1 字(词)/分钟\n"
                "最高速度：%2 字(词)/分钟\n"
                "击键速度：%3 键/秒\n"
                "测试成绩：%4（速度 - 错误数）\n"
                "正确率：%5%\n"
                "错误数：%6\n"
                "用时：%7")
            .arg(rec.speed)
            .arg(rec.bestSpeed)
            .arg(rec.hitSpeed, 0, 'f', 2)
            .arg(rec.score())
            .arg(rec.accuracy)
            .arg(rec.wrongCount)
            .arg(SpeedTracker::formatDuration(rec.totalMs)));

    // 回到训练模式
    m_statusBar->setMode("单元训练");
    loadUnit(m_currentLibNo);
}

void MainWindow::saveTestHistory()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString path = QDir(appDir).filePath("data/test_history.xml");
    m_testHistory.save(path);
}

void MainWindow::onTrainClicked()
{
    switchToNextMode();
}

void MainWindow::switchToNextMode()
{
    // 循环：Train -> Test -> Train
    switch (m_learnMode) {
    case LearnMode::Train:
        applyLearnMode(LearnMode::Test);
        break;
    case LearnMode::Test:
        applyLearnMode(LearnMode::Train);
        break;
    }
}

void MainWindow::applyLearnMode(LearnMode mode)
{
    m_learnMode = mode;

    switch (mode) {
    case LearnMode::Train:
        // 回到普通训练
        m_questionPanel->setMode(QuestionPanel::Mode::Train);
        loadUnit(m_currentLibNo);
        break;

    case LearnMode::Test:
        // 进入测试模式
        enterTestMode();
        break;
    }

    updateStatusBarMode();

    saveUserData();
}

void MainWindow::updateStatusBarMode()
{
    switch (m_learnMode) {
    case LearnMode::Train: m_statusBar->setMode("单元训练"); break;
    case LearnMode::Test:  m_statusBar->setMode("单元测试"); break;
    }
}

void MainWindow::onSelectLibClicked()
{
    UnitSelectDialog dlg(m_userData, this);
    if (dlg.exec() == QDialog::Accepted) {
        const int libNo = dlg.selectedLibNo();
        if (libNo >= 0) {
            m_learnMode = LearnMode::Train;
            loadUnit(libNo);
            updateStatusBarMode();
        }
    }
}

void MainWindow::onSetupClicked()
{
    SettingsDialog dlg(this);
    dlg.setDisplayType(m_displayType);
    dlg.setTailTrainItemsCount(m_tailTrainItemsCount);
    dlg.setAutoTailTrainCount(m_autoTailTrainCount);
    dlg.setTailTrainMaxCount(m_tailTrainMaxCount);
    dlg.setTestItemsCount(m_testItemsCount);

    if (dlg.exec() == QDialog::Accepted) {
        m_displayType = dlg.displayType();
        m_tailTrainItemsCount = dlg.tailTrainItemsCount();
        m_autoTailTrainCount = dlg.autoTailTrainCount();
        m_tailTrainMaxCount = dlg.tailTrainMaxCount();
        m_testItemsCount = dlg.testItemsCount();

        // 应用显示方式
        applyDisplayType(m_displayType);

        // 重新加载当前单元（应用新的训练参数）
        loadUnit(m_currentLibNo);

        qDebug() << "参数已更新:"
                 << "DisplayType=" << m_displayType
                 << "TrainItems=" << m_tailTrainItemsCount
                 << "Auto=" << m_autoTailTrainCount
                 << "TrainMax=" << m_tailTrainMaxCount;
    }
}

void MainWindow::applyDisplayType(int type)
{
    m_displayType = type;
    if (m_questionPanel)
        m_questionPanel->setDisplayType(type);
}

void MainWindow::onStatusClicked()
{
    TestResultDialog dlg(m_testHistory, m_currentLibNo, this);
    dlg.exec();
    saveTestHistory();
}

void MainWindow::onHelpClicked()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString path = QDir(appDir).filePath("data/help.txt");
    HelpDialog dlg(path, this);
    dlg.exec();
}

void MainWindow::onQuitClicked() { close(); }

void MainWindow::onLookupChanged(const QString &text)
{
    if (text.isEmpty()) {
        m_infoPanel->setLookupResult("");
        return;
    }

    QStringList codes = m_zmmb.lookup(text);
    if (codes.isEmpty()) {
        m_infoPanel->setLookupResult("（未找到）");
        return;
    }

    QString joined = codes.join("  ");
    if (m_displayType == 0)
        joined = joined.toLower();

    m_infoPanel->setLookupResult(joined);
}

void MainWindow::onClockTick()
{
    m_statusBar->setClock(QTime::currentTime().toString("HH:mm:ss"));
}

void MainWindow::enterTestMode()
{
    m_session.enterTestMode();

    // 一次拿到 text + code + originalIndex
    const auto entries = m_session.getTestItemsWithIndex();

    m_testItemsAll.clear();
    m_testIndicesAll.clear();
    m_testItemsAll.reserve(entries.size());
    m_testIndicesAll.reserve(entries.size());

    for (const auto &e : entries) {
        m_testItemsAll.append({e.text, e.code});
        m_testIndicesAll.append(e.originalIndex);
    }

    m_testPageOffset = 0;
    m_testTotalCorrect = 0;
    m_testTotalWrong = 0;
    m_testElapsedMs = 0;
    m_testKeyStrokes = 0;

    m_questionPanel->setMode(QuestionPanel::Mode::Test);

    TestView *tv = m_questionPanel->testView();
    if (tv) {
        m_testPageSize = tv->pageCapacity();
        if (m_testPageSize <= 0) m_testPageSize = 100;
    }

    loadTestPage();

    m_learnMode = LearnMode::Test;
    updateStatusBarMode();
}

void MainWindow::loadTestPage()
{
    TestView *tv = m_questionPanel->testView();
    if (!tv) return;

    if (m_testPageOffset >= m_testItemsAll.size()) {
        onTestFinished();
        return;
    }

    const int end = qMin(m_testPageOffset + m_testPageSize,
                         m_testItemsAll.size());

    QList<QPair<QString, QString>> page;
    QList<int> pageIndices;
    for (int i = m_testPageOffset; i < end; ++i) {
        page.append(m_testItemsAll[i]);
        pageIndices.append(m_testIndicesAll[i]);
    }

    const int totalPages = (m_testItemsAll.size() + m_testPageSize - 1)
                            / m_testPageSize;
    const int currentPage = m_testPageOffset / m_testPageSize + 1;

    tv->setGlobalCounters(m_testTotalCorrect, m_testTotalWrong);
    tv->setPageInfo(QString("第 %1/%2 页").arg(currentPage).arg(totalPages));
    tv->loadItems(page, pageIndices);
}

void MainWindow::onTestItemAnswered(bool correct, int originalIndex)
{
    // 测试答错的题，回写 speedTable，训练时加重
    m_session.reportTestAnswer(originalIndex, correct);
}

void MainWindow::onTestPageFinished()
{
    TestView *tv = m_questionPanel->testView();
    if (!tv) return;

    // 累计本页
    m_testTotalCorrect += tv->correctCount();
    m_testTotalWrong   += tv->wrongCount();
    m_testElapsedMs    += tv->elapsedMs();
    m_testKeyStrokes   += tv->keyStrokes();

    // 推进到下一页
    m_testPageOffset += m_testPageSize;

    if (m_testPageOffset >= m_testItemsAll.size()) {
        onTestFinished();
    } else {
        loadTestPage();
    }
}

void MainWindow::onTestFinished()
{
    ZbUnit *unit = m_userData.findUnit(m_currentLibNo);

    TestRecord rec;
    rec.libNo      = m_currentLibNo;
    rec.libName    = unit ? unit->libName : "";

    // 测试速度 = 完成题数 / 秒表时间（秒表计时，含停顿）
    const int itemTotal = m_testItemsAll.size();
    const int totalAns = m_testTotalCorrect + m_testTotalWrong;
    rec.speed      = (m_testElapsedMs > 0 && itemTotal > 0)
        ? static_cast<int>(60000LL * itemTotal / m_testElapsedMs)
        : 0;
    rec.bestSpeed  = rec.speed;
    // 击键速度 = 击键次数 / 时间(秒)
    rec.hitSpeed   = m_testElapsedMs > 0
        ? 1000.0 * m_testKeyStrokes / m_testElapsedMs
        : 0.0;
    rec.accuracy   = totalAns > 0
        ? 100 * m_testTotalCorrect / totalAns
        : 0;
    rec.wrongCount = m_testTotalWrong;
    rec.totalMs    = m_testElapsedMs;
    rec.dateTime   = QDateTime::currentDateTime();

    m_testHistory.addRecord(rec);
    saveTestHistory();

    // 刷新 Used
    if (unit) {
        unit->used = UserData::isUnitCompleted(*unit) ? 1 : 0;
    }
    saveUserData();

    QMessageBox::information(this, "测试结束",
        QString("平均速度：%1 字(词)/分钟\n"
                "击键速度：%2 键/秒\n"
                "测试成绩：%3（速度 - 错误数）\n"
                "正确率：%4%\n"
                "错误数：%5\n"
                "用时：%6")
            .arg(rec.speed)
            .arg(rec.hitSpeed, 0, 'f', 2)
            .arg(rec.score())
            .arg(rec.accuracy)
            .arg(rec.wrongCount)
            .arg(SpeedTracker::formatDuration(rec.totalMs)));

    m_questionPanel->setMode(QuestionPanel::Mode::Train);
    m_learnMode = LearnMode::Train;
    loadUnit(m_currentLibNo);
    updateStatusBarMode();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveUserData();
    saveTestHistory();
    QMainWindow::closeEvent(event);
}

void MainWindow::onHelpRequested()
{
    const ZbItem *item = m_session.currentItem();
    if (!item) return;

    QStringList codes;
    if (!item->code.isEmpty())
        codes << item->code;
    else
        codes = m_zmmb.lookup(item->charText);

    if (codes.isEmpty()) {
        m_questionPanel->setAssociate("提示：未找到编码", "#5353c4");
    } else {
        // 按显示方式转换大小写
        QString joined = codes.join(' ');
        if (m_displayType == 0)
            joined = joined.toLower();
        m_questionPanel->setAssociate("提示：" + joined, "#5353c4");
    }

    // 求助后不提交，用户可以继续输入
    m_questionPanel->focusInput();
}