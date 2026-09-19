// UnitSelectDialog.cpp
#include "UnitSelectDialog.h"

#include <QTabWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QTextCursor>
#include <QFile>
#include <QDir>
#include <QApplication>

UnitSelectDialog::UnitSelectDialog(const UserData &userData, QWidget *parent)
    : QDialog(parent), m_userData(userData)
{
    setWindowTitle("选择单元库");
    resize(720, 520);
    
    setupUi();
    loadHelpFile();
    setupTabs();
}

void UnitSelectDialog::setupUi()
{
    // 标题
    auto *title = new QLabel("选择单元库", this);
    title->setAlignment(Qt::AlignCenter);
    QFont titleFont("KaiTi", 18, QFont::Bold);
    title->setFont(titleFont);
    title->setStyleSheet("color: #006432; padding: 8px;");

    // 左侧：Tab 页（单元列表）
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setFont(QFont("SimSun", 11));
    m_tabWidget->setMinimumWidth(260);
    m_tabWidget->setMaximumWidth(320);

    // 右侧：帮助显示
    m_textHelp = new QTextEdit(this);
    m_textHelp->setReadOnly(true);
    m_textHelp->setFont(QFont("SimSun", 11));
    m_textHelp->setStyleSheet(
        "QTextEdit {"
        "  background-color: #F8F8F8;"
        "  border: 1px solid #CCCCCC;"
        "  padding: 6px;"
        "}");

    // 中间：左右水平布局
    auto *centerLayout = new QHBoxLayout();
    centerLayout->setSpacing(8);
    centerLayout->addWidget(m_tabWidget, 0);       // 左侧固定宽度
    centerLayout->addWidget(m_textHelp, 1);        // 右侧占满剩余

    // 底部按钮
    m_btnOK = new QPushButton("开始", this);
    m_btnCancel = new QPushButton("取消", this);

    auto setupBtn = [](QPushButton *btn) {
        btn->setFixedHeight(34);
        btn->setMinimumWidth(100);
        btn->setFont(QFont("SimSun", 11));
    };
    setupBtn(m_btnOK);
    setupBtn(m_btnCancel);

    connect(m_btnOK,     &QPushButton::clicked, this, &UnitSelectDialog::onOKClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &UnitSelectDialog::onCancelClicked);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnOK);
    btnLayout->addWidget(m_btnCancel);

    // 总布局（垂直）
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 12);
    layout->setSpacing(8);
    layout->addWidget(title);
    layout->addLayout(centerLayout, 1);   // 中间左右布局占满
    layout->addLayout(btnLayout);
}

// 统一单元名：去首尾空白（含全角空格）
QString UnitSelectDialog::normalizeName(const QString &name)
{
    QString s = name;
    s = s.trimmed();
    // 去掉全角空格
    s.remove(QChar(0x3000));
    return s;
}

void UnitSelectDialog::loadHelpFile()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString path = QDir(appDir).filePath("data/UnitHelp.txt");

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开 UnitHelp.txt:" << path;
        return;
    }

    QDomDocument doc;
    const auto result = doc.setContent(&file);
    file.close();
    if (!result) {
        qWarning() << "UnitHelp.txt 解析失败:" << result.errorMessage;
        return;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != "UnitHelp") return;

    QDomElement texts = root.firstChildElement("Texts");
    for (QDomElement item = texts.firstChildElement("Item");
         !item.isNull();
         item = item.nextSiblingElement("Item"))
    {
        const QString name = item.attribute("Name");
        if (name.isEmpty()) continue;

        const QString key = normalizeName(name);
        if (key.isEmpty()) continue;

        const QString content = item.text();
        if (content.isEmpty()) continue;

        // 同名保留第一个（或覆盖，视需要）
        if (!m_helpMap.contains(key))
            m_helpMap.insert(key, content);
    }

    qDebug() << "UnitHelp.txt 加载完成，共" << m_helpMap.size() << "条";
}

QString UnitSelectDialog::helpForUnit(const QString &unitName) const
{
    const QString key = normalizeName(unitName);
    return m_helpMap.value(key, QString());
}

UnitSelectDialog::Category UnitSelectDialog::categorize(const ZbUnit &unit) const
{
    const int no = unit.libNo;

    // 字根：0, 2, 10~19
    if (no == 0 || no == 2 || (no >= 10 && no <= 19))
        return CatRoot;

    // 简码：20~34
    if (no >= 20 && no <= 34)
        return CatSimple;

    // 词汇：50~53
    if (no >= 50 && no <= 53)
        return CatWord;

    // 常用：100~105
    if (no >= 100 && no <= 105)
        return CatCommon;

    // 提速：200~207
    if (no >= 200 && no <= 207)
        return CatSpeed;

    // 默认归到字根
    return CatRoot;
}

void UnitSelectDialog::setupTabs()
{
    // 创建 5 个 Tab
    const QString tabNames[5] = {"字根单元", "简码单元", "词汇单元", "常用单元", "提速单元"};

    for (int i = 0; i < 5; ++i) {
        auto *list = new QListWidget(m_tabWidget);
        list->setFont(QFont("SimSun", 12));
        list->setSpacing(2);
        list->setStyleSheet(
            "QListWidget::item { padding: 6px; } "
            "QListWidget::item:selected {"
            "  background-color: palette(highlight);"
            "  color: palette(highlighted-text);"
            "}");
        m_listWidgets[i] = list;
        m_tabWidget->addTab(list, tabNames[i]);

        connect(list, &QListWidget::itemClicked,
                this, &UnitSelectDialog::onUnitClicked);
    }

    // 填充单元
    for (const ZbUnit &unit : m_userData.units()) {
        const Category cat = categorize(unit);
        addUnitToTab(static_cast<int>(cat), unit);
    }

    // 默认选中第一个 Tab 的第一项
    if (m_listWidgets[0]->count() > 0) {
        m_listWidgets[0]->setCurrentRow(0);
        onUnitClicked(m_listWidgets[0]->item(0));
    }
}

void UnitSelectDialog::addUnitToTab(int tabIndex, const ZbUnit &unit)
{
    if (tabIndex < 0 || tabIndex >= 5) return;

    auto *item = new QListWidgetItem(
        QString("%1  (%2 题)").arg(unit.libName).arg(unit.items.size()));
    item->setData(Qt::UserRole, unit.libNo);

    m_listWidgets[tabIndex]->addItem(item);
}

void UnitSelectDialog::onUnitClicked(QListWidgetItem *item)
{
    if (!item) return;

    const int libNo = item->data(Qt::UserRole).toInt();
    m_selectedLibNo = libNo;

    // 查找单元
    for (const ZbUnit &unit : m_userData.units()) {
        if (unit.libNo == libNo) {
            updateHelp(unit);
            break;
        }
    }
}

void UnitSelectDialog::updateHelp(const ZbUnit &unit)
{
    // 头部信息
    QString html;
    html += QString("<p style='margin:0 0 8px 0;'>"
                    "<b>单元名称：</b>%1<br>"
                    "<b>单元编号：</b>%2<br>"
                    "<b>题目数量：</b>%3</p>")
            .arg(unit.libName.toHtmlEscaped())
            .arg(unit.libNo)
            .arg(unit.items.size());

    // ★ 从 UnitHelp.txt 取说明
    QString help = helpForUnit(unit.libName);
    if (help.isEmpty()) {
        help = "（暂无该单元的说明）";
    } else {
        // 转义 HTML，保留换行
        help.replace("&", "&amp;");
        help.replace("<", "&lt;");
        help.replace(">", "&gt;");
        help.replace("\n", "<br>");
    }

    html += "<hr>";
    html += QString(
        "<pre style='font-family:zmzg; font-size:12pt; "
        "white-space:pre-wrap; margin:0;'>%1</pre>")
        .arg(help);
    m_textHelp->setHtml(html);
}

void UnitSelectDialog::onOKClicked()
{
    if (m_selectedLibNo < 0) {
        return;
    }
    accept();
}

void UnitSelectDialog::onCancelClicked()
{
    m_selectedLibNo = -1;
    reject();
}