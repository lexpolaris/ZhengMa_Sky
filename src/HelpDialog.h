// HelpDialog.h
#pragma once
#include <QDialog>
#include <QList>

class QListWidget;
class QTextEdit;
class QPushButton;

class HelpDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HelpDialog(const QString &helpFilePath, QWidget *parent = nullptr);

private slots:
    void onTopicClicked(int row);

private:
    void setupUi();
    bool loadHelp(const QString &filePath);

    QListWidget *m_listTopics = nullptr;
    QTextEdit   *m_textHelp = nullptr;
    QPushButton *m_btnBack = nullptr;

    QStringList m_topicNames;
    QStringList m_topicContents;
};