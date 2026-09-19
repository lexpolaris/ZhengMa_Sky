// HelpDialog.cpp
#include "HelpDialog.h"
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QFile>
#include <QDomDocument>
#include <QDebug>

HelpDialog::HelpDialog(const QString &helpFilePath, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("帮助");
    resize(820, 560);
    setupUi();

    if (!loadHelp(helpFilePath)) {
        m_textHelp->setHtml("<p style='color:#888;'>帮助文件加载失败</p>");
    }
}

void HelpDialog::setupUi()
{
    QFont songFont("SimSun", 11);
    QFont titleFont("KaiTi", 16, QFont::Bold);

    // 左侧目录
    m_listTopics = new QListWidget(this);
    m_listTopics->setFont(QFont("SimSun", 12));
    m_listTopics->setFixedWidth(180);
    m_listTopics->setStyleSheet(
        "QListWidget::item { padding: 8px; } "
        "QListWidget::item:selected {"
        "  background-color: palette(highlight);"
        "  color: palette(highlighted-text);"
        "}");

    connect(m_listTopics, &QListWidget::currentRowChanged,
            this, &HelpDialog::onTopicClicked);

    // 右侧内容
    m_textHelp = new QTextEdit(this);
    m_textHelp->setReadOnly(true);
    m_textHelp->setFont(songFont);
    m_textHelp->setStyleSheet(
        "QTextEdit {"
        "  background-color: palette(base);"
        "  color: palette(text);"
        "  border: 1px solid palette(mid);"
        "}");

    // 标题
    auto *title = new QLabel("帮助", this);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    {
        QPalette p = title->palette();
        p.setColor(QPalette::WindowText, p.color(QPalette::Highlight));
        title->setPalette(p);
    }

    // 返回按钮
    m_btnBack = new QPushButton("返回", this);
    m_btnBack->setFixedHeight(32);
    m_btnBack->setMinimumWidth(80);
    connect(m_btnBack, &QPushButton::clicked, this, &QDialog::accept);

    // 中间布局
    auto *midLayout = new QHBoxLayout();
    midLayout->addWidget(m_listTopics);
    midLayout->addWidget(m_textHelp, 1);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnBack);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 12);
    layout->addWidget(title);
    layout->addLayout(midLayout, 1);
    layout->addLayout(btnLayout);
}

bool HelpDialog::loadHelp(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开帮助文件:" << filePath;
        return false;
    }

    QDomDocument doc;
    const auto result = doc.setContent(&file);
    file.close();

    if (!result) {
        qWarning() << "帮助文件解析失败:" << result.errorMessage;
        return false;
    }

    QDomElement root = doc.documentElement();   // <Help>
    if (root.tagName() != "Help") return false;

    // 读 <Contents> 里的 <Item>
    QDomElement contents = root.firstChildElement("Contents");
    for (QDomElement item = contents.firstChildElement("Item");
         !item.isNull();
         item = item.nextSiblingElement("Item"))
    {
        QString name = item.attribute("Name");
        m_topicNames.append(name);
    }

    // 读 <Texts> 里的 <Item>
    QDomElement texts = root.firstChildElement("Texts");
    QHash<QString, QString> contentMap;
    for (QDomElement item = texts.firstChildElement("Item");
         !item.isNull();
         item = item.nextSiblingElement("Item"))
    {
        QString name = item.attribute("Name");
        QString content = item.text();
        contentMap.insert(name, content);
    }

    // 按 Contents 的顺序填充内容和目录
    for (const QString &name : m_topicNames) {
        m_listTopics->addItem(name);
        m_topicContents.append(contentMap.value(name, "(暂无内容)"));
    }

    if (m_listTopics->count() > 0) {
        m_listTopics->setCurrentRow(0);
    }

    return true;
}

void HelpDialog::onTopicClicked(int row)
{
    if (row < 0 || row >= m_topicContents.size()) return;

    QString content = m_topicContents.at(row);
    // 转义 HTML，保留换行
    content.replace("&", "&amp;")
           .replace("<", "&lt;")
           .replace(">", "&gt;")
           .replace("\n", "<br>");

    m_textHelp->setHtml(QString("<div style='font-family:SimSun;font-size:11pt;"
                                "line-height:1.6;'>%1</div>").arg(content));
}