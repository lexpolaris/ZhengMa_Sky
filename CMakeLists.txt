cmake_minimum_required(VERSION 3.16)
project(zhengma-sky VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Xml Charts)

add_executable(zhengma-sky
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

add_custom_command(TARGET zhengma-sky POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:zhengma-sky>/data
)
