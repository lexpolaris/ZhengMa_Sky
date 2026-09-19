// QuestionPanel.cpp
#include "QuestionPanel.h"
#include "TestView.h"
#include <QVBoxLayout>
#include <QKeyEvent>

QuestionPanel::QuestionPanel(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    
    m_stack = new QStackedWidget(this);

    setupTrainView();
    setupTestView();

    m_stack->addWidget(m_trainView);
    m_stack->addWidget(m_testView);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stack);

    setMode(Mode::Train);
}

void QuestionPanel::setupTrainView()
{
    m_trainView = new QWidget(this);

    // 题目显示
    m_labelQuestion = new QLabel(m_trainView);
    m_labelQuestion->setAlignment(Qt::AlignCenter);
    m_labelQuestion->setMinimumHeight(180);
    {
        QPalette p = m_labelQuestion->palette();
        p.setColor(QPalette::WindowText, p.color(QPalette::Highlight));
        m_labelQuestion->setPalette(p);
    }
    QFont qFont;
    qFont.setFamily("zmzg");
    qFont.setPointSize(36);
    m_labelQuestion->setFont(qFont);

    // 编码提示
    m_labelHint = new QLabel(m_trainView);
    m_labelHint->setAlignment(Qt::AlignCenter);
    {
        QPalette p = m_labelHint->palette();
        // 用 Link 色作为提示色（默认蓝，可在 main 里改）
        p.setColor(QPalette::WindowText, p.color(QPalette::Link));
        m_labelHint->setPalette(p);
    }    m_labelHint->setFont(QFont("SimSun", 20));

    // 输入框
    m_editInput = new QLineEdit(m_trainView);
    m_editInput->setAlignment(Qt::AlignCenter);
    m_editInput->setFont(QFont("Arial", 24));
    m_editInput->setFixedHeight(56);
    m_editInput->setStyleSheet(
        "QLineEdit {"
        "  border: 2px solid palette(mid);"
        "  border-radius: 6px;"
        "  background-color: palette(base);"
        "  color: palette(text);"
        "  padding: 4px;"
        "}"
        "QLineEdit:focus {"
        "  border-color: palette(highlight);"
        "}");
    m_editInput->installEventFilter(this);

    // 联想提示
    m_labelAssociate = new QLabel(m_trainView);
    m_labelAssociate->setAlignment(Qt::AlignCenter);
    m_labelAssociate->setStyleSheet("color: #006432;");
    m_labelAssociate->setFont(QFont("SimSun", 14));
    m_labelAssociate->setWordWrap(true);
    m_labelAssociate->setMinimumHeight(60);

    auto *layout = new QVBoxLayout(m_trainView);
    layout->setContentsMargins(40, 20, 40, 20);
    layout->setSpacing(16);
    layout->addStretch(1);
    layout->addWidget(m_labelQuestion, 3);
    layout->addWidget(m_labelHint, 1);
    layout->addSpacing(8);
    layout->addWidget(m_editInput);
    layout->addWidget(m_labelAssociate, 1);
    layout->addStretch(1);
}

void QuestionPanel::setupTestView()
{
    m_testView = new TestView(this);
}

void QuestionPanel::setMode(Mode mode)
{
    if (m_mode == mode) return;
    m_mode = mode;
    applyModeLayout();
}

void QuestionPanel::applyModeLayout()
{
    switch (m_mode) {
    case Mode::Train:
        m_stack->setCurrentWidget(m_trainView);
        m_labelHint->setVisible(true);
        m_labelAssociate->setVisible(true);
        break;

    case Mode::Test:
        m_stack->setCurrentWidget(m_testView);
        break; 

    case Mode::Speed:
        m_stack->setCurrentWidget(m_trainView);
        m_labelHint->setVisible(true);
        m_labelAssociate->setVisible(false);
        break;
    }
}

// ===== 训练模式接口 =====

void QuestionPanel::setQuestion(const QString &text)
{
    m_labelQuestion->setText(text);
}

void QuestionPanel::setHint(const QString &text)
{
    m_labelHint->setText(m_displayType == 1 ? text.toUpper() : text.toLower());
}

void QuestionPanel::setAssociate(const QString &text, const QString &color)
{
    QString display = text;
    if (m_displayType == 1)
        display = display.toUpper();
    else
        display = display.toLower();

    if (color.isEmpty()) {
        m_labelAssociate->setText(display);
    } else {
        // 用 HTML 富文本，指定颜色
        m_labelAssociate->setText(
            QString("<span style='color:%1;'>%2</span>")
                .arg(color, display.toHtmlEscaped()));
    }
}

void QuestionPanel::clearInput()
{
    m_editInput->clear();
}

QString QuestionPanel::inputText() const
{
    return m_editInput->text();
}

void QuestionPanel::focusInput()
{
    if (m_mode == Mode::Train || m_mode == Mode::Speed)
        m_editInput->setFocus();
    else if (m_testView)
        m_testView->setFocus();
}

void QuestionPanel::setInputEnabled(bool enabled)
{
    m_editInput->setEnabled(enabled);
}

void QuestionPanel::setQuestionFont(const QFont &font)
{
    m_labelQuestion->setFont(font);
}

void QuestionPanel::setDisplayType(int type)
{
    m_displayType = type;
    // 后续可加大小写验证器
}

void QuestionPanel::loadTestItems(const QList<QPair<QString, QString>> &items,
                                   const QList<int> &originalIndices)
{
    if (m_testView)
        m_testView->loadItems(items, originalIndices);
}

// void QuestionPanel::keyPressEvent(QKeyEvent *event)
// {
//     QWidget::keyPressEvent(event);
// }

bool QuestionPanel::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_editInput && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);

        // 空格：空输入求助，非空输入提交
        if (ke->key() == Qt::Key_Space) {
            if (m_editInput->text().isEmpty()) {
                emit helpRequested();
            } else {
                emit inputSubmitted(m_editInput->text());
            }
            return true;
        }

        // 回车：直接提交
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            emit inputSubmitted(m_editInput->text());
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}