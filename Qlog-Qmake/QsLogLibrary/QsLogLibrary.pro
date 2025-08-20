# 定义项目模板为应用程序
#TEMPLATE = app
#QT += core widgets sql

## 配置项目，启用 C++11 支持
#CONFIG += c++11



# 模板类型：lib，表示这是一个库
TEMPLATE = lib

# 目标文件名，最终会生成 QsLogLibrary.dll (Windows) 或 libQsLogLibrary.so (Linux)
TARGET = QsLogLibrary

# 启用 C++11 标准
CONFIG += c++11

# 启用 Qt 的核心和部件模块
QT += core widgets sql

# 这条配置是关键！
# 它告诉 qmake 为这个库项目生成导出/导入宏
# 库自身在编译时会定义 QSLOGLIBRARY_EXPORTS，使得 Q_DECL_EXPORT 生效
# 使用这个库的项目在链接时会使用 Q_DECL_IMPORT
CONFIG += QsLogLibrary_EXPORTS





# 定义项目的源文件
SOURCES += \
    #main.cpp \
    QsLog.cpp \
    QsLogDest.cpp \
    QsLogDestConsole.cpp \
    QsLogDestFile.cpp \
    QsLogDestFunctor.cpp

# 定义项目的头文件
HEADERS += \
    QsLog.h \
    QsLogDest.h \
    QsLogDestConsole.h \
    QsLogDestFile.h \
    QsLogDestFunctor.h \
    QsLogDisableForThisFile.h \
    QsLogLevel.h \
    QsLogLibrary_global.h


