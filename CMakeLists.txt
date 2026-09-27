cmake_minimum_required(VERSION 3.16)
project(zhengma-sky VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Xml Charts)

add_executable(zhengma-sky
    resources.qrc
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/InfoPanel.cpp
    src/InfoPanel.h
    src/QuestionPanel.cpp
    src/QuestionPanel.h
    src/StatusBar.cpp
    src/StatusBar.h
    src/UserData.cpp
    src/UserData.h
    src/ZmmbTable.cpp
    src/ZmmbTable.h
    src/Judge.h
    src/Judge.cpp
    src/QuestionPool.h
    src/QuestionPool.cpp
    src/SpeedTracker.h
    src/SpeedTracker.cpp
    src/UnitSession.h
    src/UnitSession.cpp
    src/UnitSelectDialog.h
    src/UnitSelectDialog.cpp
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/TestHistory.cpp
    src/TestHistory.h
    src/TestResultDialog.cpp
    src/TestResultDialog.h
    src/HelpDialog.cpp
    src/HelpDialog.h
    src/TestView.h
    src/TestView.cpp
)

target_link_libraries(zhengma-sky PRIVATE
    Qt6::Widgets
    Qt6::Xml
    Qt6::Charts
)

# 可执行文件所需的静态素材（help.txt / train.txt / UnitHelp.txt / zmmb.txt / zmzg.ttf）
# 已通过 resources.qrc 内嵌到二进制中，无需再复制 data 目录。
# 用户可写数据（user.xml / test_history.xml）运行时写入 QStandardPaths::AppDataLocation。

