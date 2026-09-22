// SettingsDialog.h
#pragma once
#include <QDialog>

class QRadioButton;
class QButtonGroup;
class QSpinBox;
class QCheckBox;
class QPushButton;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // 设置初始值
    void setDisplayType(int type);              // 0=小写, 1=大写
    void setTailTrainItemsCount(int count);     // 末位训练数量
    void setAutoTailTrainCount(bool autoMode);  // 自动确定
    void setTailTrainMaxCount(int count);       // 末位训练次数

    // 获取结果
    int displayType() const;
    int tailTrainItemsCount() const;
    bool autoTailTrainCount() const;
    int tailTrainMaxCount() const;

    void setTestItemsCount(int count);   // -1 = 默认（自动）
    int testItemsCount() const;


private slots:
    void onAutoChanged(bool checked);
    void onOKClicked();
    void onCancelClicked();

private:
    void setupUi();

    QButtonGroup *m_displayGroup = nullptr;
    QRadioButton *m_rbUpper = nullptr;      // 大写
    QRadioButton *m_rbLower = nullptr;      // 小写

    QSpinBox *m_spinTrainItems = nullptr;   // 末位训练数量

    QCheckBox *m_checkAuto = nullptr;       // 自动确定
    QSpinBox *m_spinTrainMax = nullptr;     // 末位训练次数

    QSpinBox *m_spinTestItems = nullptr;
    QCheckBox *m_checkTestAuto = nullptr;

    QPushButton *m_btnOK = nullptr;
    QPushButton *m_btnCancel = nullptr;

    
};