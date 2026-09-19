#pragma once
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>
#include <QFont>

class TestView;

class QuestionPanel : public QWidget
{
    Q_OBJECT
public:
    enum class Mode {
        Train,
        Test,
        Speed,
    };

    explicit QuestionPanel(QWidget *parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    // 训练模式接口
    void setQuestion(const QString &text);
    void setHint(const QString &text);
    void setAssociate(const QString &text, const QString &color = QString());
    void clearInput();
    QString inputText() const;
    void focusInput();

    void setInputEnabled(bool enabled);
    void setQuestionFont(const QFont &font);

    // 测试/游戏模式接口
    void loadTestItems(const QList<QPair<QString, QString>> &items,
                       const QList<int> &originalIndices);
    TestView* testView() const { return m_testView; }

    // 显示方式：0=小写, 1=大写
    void setDisplayType(int type);
    int displayType() const { return m_displayType; }
    
signals:
    void inputSubmitted(const QString &text);
    void helpRequested(); 

protected:
    // void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupTrainView();
    void setupTestView();
    void applyModeLayout();

    Mode m_mode = Mode::Train;

    QStackedWidget *m_stack = nullptr;

    // 训练视图
    QWidget *m_trainView = nullptr;
    QLabel  *m_labelQuestion = nullptr;
    QLabel  *m_labelHint = nullptr;
    QLabel  *m_labelAssociate = nullptr;
    QLineEdit *m_editInput = nullptr;

    // 测试/游戏视图
    TestView *m_testView = nullptr;

    int m_displayType = 0;
};