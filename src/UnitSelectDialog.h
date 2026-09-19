// UnitSelectDialog.h
#pragma once
#include <QDialog>
#include <QList>
#include "UserData.h"

class QTabWidget;
class QListWidget;
class QListWidgetItem;
class QTextEdit;
class QPushButton;

class UnitSelectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UnitSelectDialog(const UserData &userData, QWidget *parent = nullptr);

    // 选中的单元编号（-1 表示取消）
    int selectedLibNo() const { return m_selectedLibNo; }

private slots:
    void onUnitClicked(QListWidgetItem *item);
    void onOKClicked();
    void onCancelClicked();

private:
    void setupUi();
    void setupTabs();
    void addUnitToTab(int tabIndex, const ZbUnit &unit);
    void updateHelp(const ZbUnit &unit);

    void loadHelpFile();
    QString helpForUnit(const QString &unitName) const;
    static QString normalizeName(const QString &name);

    // 单元分类
    enum Category {
        CatRoot = 0,    // 字根
        CatSimple,      // 简码
        CatWord,        // 词汇
        CatCommon,      // 常用
        CatSpeed        // 提速
    };
    Category categorize(const ZbUnit &unit) const;

    const UserData &m_userData;
    QTabWidget  *m_tabWidget = nullptr;
    QListWidget *m_listWidgets[5] = {nullptr};
    QTextEdit   *m_textHelp = nullptr;
    QPushButton *m_btnOK = nullptr;
    QPushButton *m_btnTest = nullptr;
    QPushButton *m_btnCancel = nullptr;

    int  m_selectedLibNo = -1;
    
     // 单元名 -> 帮助内容
    QHash<QString, QString> m_helpMap;
};