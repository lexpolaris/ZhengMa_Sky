#include <QApplication>
#include <QFontDatabase>
#include <QDir>
#include <QStyleFactory>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("郑码天空");
    app.setOrganizationName("ZhengMaSky");

    // ---- 统一调色板：浅色，但所有颜色走 QPalette 角色 ----
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette pal;
    pal.setColor(QPalette::Window,          QColor(0xF5, 0xF5, 0xF5));
    pal.setColor(QPalette::WindowText,      QColor(0x20, 0x20, 0x20));
    pal.setColor(QPalette::Base,            QColor(0xFF, 0xFF, 0xFF));
    pal.setColor(QPalette::AlternateBase,   QColor(0xF0, 0xF0, 0xF0));
    pal.setColor(QPalette::Text,            QColor(0x20, 0x20, 0x20));
    pal.setColor(QPalette::Button,          QColor(0xE8, 0xE8, 0xE8));
    pal.setColor(QPalette::ButtonText,      QColor(0x20, 0x20, 0x20));
    pal.setColor(QPalette::Highlight,       QColor(0x00, 0x64, 0x32));  // 主题绿
    pal.setColor(QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));
    pal.setColor(QPalette::ToolTipBase,     QColor(0xFF, 0xFF, 0xDC));
    pal.setColor(QPalette::ToolTipText,     QColor(0x20, 0x20, 0x20));
    pal.setColor(QPalette::PlaceholderText, QColor(0x88, 0x88, 0x88));
    pal.setColor(QPalette::Disabled, QPalette::Text,       QColor(0xA0, 0xA0, 0xA0));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0xA0, 0xA0, 0xA0));
    app.setPalette(pal);

    // 加载字根字体
    QString fontPath = QDir(QCoreApplication::applicationDirPath())
                           .filePath("data/zmzg.ttf");
    int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId >= 0) {
        QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            qDebug() << "已加载字根字体:" << families.at(0);
        }
    } else {
        qWarning() << "未能加载字根字体:" << fontPath;
    }

    MainWindow w;
    w.show();
    return app.exec();
}
