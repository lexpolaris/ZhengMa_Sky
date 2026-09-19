// SettingsDialog.cpp
#include "SettingsDialog.h"
#include <QRadioButton>
#include <QButtonGroup>
#include <QKeySequenceEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFont>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("参数设置");
    resize(420, 480);
    setupUi();
}

void SettingsDialog::setupUi()
{
    QFont songFont("SimSun", 11);
    QFont titleFont("KaiTi", 16, QFont::Bold);

    // 标题
    auto *title = new QLabel("参数设置", this);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    {
        QPalette p = title->palette();
        p.setColor(QPalette::WindowText, p.color(QPalette::Highlight));
        title->setPalette(p);
    }
    // === 显示方式 ===
    auto *groupDisplay = new QGroupBox("显示方式", this);
    groupDisplay->setFont(songFont);

    m_rbUpper = new QRadioButton("大写显示", groupDisplay);
    m_rbLower = new QRadioButton("小写显示", groupDisplay);
    m_rbUpper->setFont(songFont);
    m_rbLower->setFont(songFont);
    m_rbLower->setChecked(true);

    m_displayGroup = new QButtonGroup(this);
    m_displayGroup->addButton(m_rbUpper, 1);
    m_displayGroup->addButton(m_rbLower, 0);

    auto *displayLayout = new QVBoxLayout(groupDisplay);
    displayLayout->addWidget(m_rbUpper);
    displayLayout->addWidget(m_rbLower);

    // === 快捷键定义 ===
    auto *groupHotKey = new QGroupBox("快捷键定义", this);
    groupHotKey->setFont(songFont);

    auto *hotKeyLabel = new QLabel("显示/隐藏窗口：", groupHotKey);
    hotKeyLabel->setFont(songFont);

    m_hotKeyEdit = new QKeySequenceEdit(groupHotKey);
    m_hotKeyEdit->setFont(songFont);
    m_hotKeyEdit->setKeySequence(QKeySequence(Qt::Key_F10));

    auto *hotKeyLayout = new QVBoxLayout(groupHotKey);
    hotKeyLayout->addWidget(hotKeyLabel);
    hotKeyLayout->addWidget(m_hotKeyEdit);

    // === 末位训练数量 ===
    auto *groupTrainItems = new QGroupBox("末位训练数量", this);
    groupTrainItems->setFont(songFont);

    auto *itemsLabel = new QLabel("数量：", groupTrainItems);
    itemsLabel->setFont(songFont);

    m_spinTrainItems = new QSpinBox(groupTrainItems);
    m_spinTrainItems->setFont(songFont);
    m_spinTrainItems->setRange(11, 1000);
    m_spinTrainItems->setValue(5);
    m_spinTrainItems->setSuffix(" 题");

    auto *itemsLayout = new QHBoxLayout(groupTrainItems);
    itemsLayout->addWidget(itemsLabel);
    itemsLayout->addWidget(m_spinTrainItems);
    itemsLayout->addStretch();

    // === 末位训练次数 ===
    auto *groupTrainMax = new QGroupBox("末位训练次数", this);
    groupTrainMax->setFont(songFont);

    m_checkAuto = new QCheckBox("自动确定", groupTrainMax);
    m_checkAuto->setFont(songFont);
    m_checkAuto->setChecked(true);

    auto *maxLabel = new QLabel("次数：", groupTrainMax);
    maxLabel->setFont(songFont);

    m_spinTrainMax = new QSpinBox(groupTrainMax);
    m_spinTrainMax->setFont(songFont);
    m_spinTrainMax->setRange(1, 1000);
    m_spinTrainMax->setValue(3);
    m_spinTrainMax->setSuffix(" 次");
    m_spinTrainMax->setEnabled(false);   // 默认自动，禁用

    connect(m_checkAuto, &QCheckBox::toggled,
            this, &SettingsDialog::onAutoChanged);

    auto *maxLayout = new QHBoxLayout(groupTrainMax);
    maxLayout->addWidget(m_checkAuto);
    maxLayout->addWidget(maxLabel);
    maxLayout->addWidget(m_spinTrainMax);
    maxLayout->addStretch();

    // === 测试题目数量 ===
    auto *groupTestItems = new QGroupBox("测试题目数量", this);
    groupTestItems->setFont(songFont);

    m_checkTestAuto = new QCheckBox("自动确定", groupTestItems);
    m_checkTestAuto->setFont(songFont);
    m_checkTestAuto->setChecked(true);

    auto *testItemsLabel = new QLabel("数量：", groupTestItems);
    testItemsLabel->setFont(songFont);

    m_spinTestItems = new QSpinBox(groupTestItems);
    m_spinTestItems->setFont(songFont);
    m_spinTestItems->setRange(20, 10000);
    m_spinTestItems->setValue(100);
    m_spinTestItems->setSuffix(" 题");
    m_spinTestItems->setEnabled(false);

    connect(m_checkTestAuto, &QCheckBox::toggled, this, [this](bool checked) {
        m_spinTestItems->setEnabled(!checked);
    });

    auto *testItemsLayout = new QHBoxLayout(groupTestItems);
    testItemsLayout->addWidget(m_checkTestAuto);
    testItemsLayout->addWidget(testItemsLabel);
    testItemsLayout->addWidget(m_spinTestItems);
    testItemsLayout->addStretch();   

    // === 按钮 ===
    m_btnOK = new QPushButton("确认", this);
    m_btnCancel = new QPushButton("取消", this);
    m_btnOK->setFont(songFont);
    m_btnCancel->setFont(songFont);
    m_btnOK->setFixedHeight(34);
    m_btnCancel->setFixedHeight(34);

    connect(m_btnOK, &QPushButton::clicked, this, &SettingsDialog::onOKClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &SettingsDialog::onCancelClicked);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnOK);
    btnLayout->addWidget(m_btnCancel);

    // === 总布局 ===
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 8, 16, 16);
    layout->setSpacing(10);
    layout->addWidget(title);
    layout->addWidget(groupDisplay);
    layout->addWidget(groupHotKey);
    layout->addWidget(groupTrainItems);
    layout->addWidget(groupTrainMax);
    layout->addWidget(groupTestItems);
    layout->addStretch();
    layout->addLayout(btnLayout);
}

void SettingsDialog::onAutoChanged(bool checked)
{
    m_spinTrainMax->setEnabled(!checked);
}

void SettingsDialog::onOKClicked() { accept(); }
void SettingsDialog::onCancelClicked() { reject(); }

// === 设置初始值 ===
void SettingsDialog::setDisplayType(int type)
{
    if (type == 1) m_rbUpper->setChecked(true);
    else           m_rbLower->setChecked(true);
}

void SettingsDialog::setHotKey(int key)
{
    if (key > 0)
        m_hotKeyEdit->setKeySequence(QKeySequence(key));
}

void SettingsDialog::setTailTrainItemsCount(int count)
{
    if (count > 0)
        m_spinTrainItems->setValue(count);
}

void SettingsDialog::setAutoTailTrainCount(bool autoMode)
{
    m_checkAuto->setChecked(autoMode);
    m_spinTrainMax->setEnabled(!autoMode);
}

void SettingsDialog::setTailTrainMaxCount(int count)
{
    if (count > 0)
        m_spinTrainMax->setValue(count);
}

// === 获取结果 ===
int SettingsDialog::displayType() const
{
    return m_displayGroup->checkedId();   // 0=小写, 1=大写
}

int SettingsDialog::hotKey() const
{
    QKeySequence seq = m_hotKeyEdit->keySequence();
    if (seq.isEmpty()) return 0;
    return seq[0].toCombined();
}

int SettingsDialog::tailTrainItemsCount() const
{
    return m_spinTrainItems->value();
}

bool SettingsDialog::autoTailTrainCount() const
{
    return m_checkAuto->isChecked();
}

int SettingsDialog::tailTrainMaxCount() const
{
    return m_spinTrainMax->value();
}

void SettingsDialog::setTestItemsCount(int count)
{
    if (count < 0) {
        m_checkTestAuto->setChecked(true);
        m_spinTestItems->setEnabled(false);
    } else {
        m_checkTestAuto->setChecked(false);
        m_spinTestItems->setEnabled(true);
        m_spinTestItems->setValue(count);
    }
}

int SettingsDialog::testItemsCount() const
{
    return m_checkTestAuto->isChecked() ? -1 : m_spinTestItems->value();
}