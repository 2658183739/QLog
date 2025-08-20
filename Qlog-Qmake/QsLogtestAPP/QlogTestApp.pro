# TEMPLATE = app
TEMPLATE = app

# 目标文件名
TARGET = QlogTestApp

# 添加所需的 Qt 模块
QT += core widgets sql

# 启用 C++11 支持
CONFIG += c++11

# 只包含应用程序自身的源文件
SOURCES += \
    main.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./ -lQsLogLibrary
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./ -lQsLogLibrary
else:unix: LIBS += -L$$PWD/./ -lQsLogLibrary

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

HEADERS += \
    QsLog.h \
    QsLogDest.h \
    QsLogDestConsole.h \
    QsLogDestFile.h \
    QsLogDestFunctor.h \
    QsLogDisableForThisFile.h \
    QsLogLevel.h
