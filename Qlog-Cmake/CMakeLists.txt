cmake_minimum_required(VERSION 3.19)
project(Qlog LANGUAGES CXX)

if(Qt6_FOUND)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql)

    qt_standard_project_setup()

    qt_add_executable(Qlog
        WIN32 MACOSX_BUNDLE
        main.cpp
        QsLog.cpp QsLog.h
        QsLogDest.cpp
        QsLogDest.h
        QsLogDestConsole.cpp
        QsLogDestConsole.h
        QsLogDestFile.cpp
        QsLogDestFile.h
        QsLogDestFunctor.cpp
        QsLogDestFunctor.h
        QsLogDisableForThisFile.h
        QsLogLevel.h

    )

    target_link_libraries(Qlog
        PRIVATE
            Qt::Core
            Qt::Widgets
            Qt::Sql
    )

    include(GNUInstallDirs)

    install(TARGETS Qlog
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    qt_generate_deploy_app_script(
        TARGET Qlog
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})


else()

    #find_package(Qt5 REQUIRED COMPONENTS Core Widgets Sql)
    find_package(Qt5 COMPONENTS Widgets LinguistTools opengl REQUIRED Sql)

    # 确保 Qt5 模块成功找到
    if (NOT Qt5_FOUND)
        message(FATAL_ERROR "Qt5 not found.")
    endif()

    # 启用当前目录包含（确保生成的moc文件能正确包含头文件）
    set(CMAKE_INCLUDE_CURRENT_DIR ON)

    # Qt专用配置：启用自动处理UI/MOC/RCC文件
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    # 设置C++11标准且必须支持
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    add_executable(Qlog
        main.cpp
        QsLog.cpp QsLog.h
        QsLogDest.cpp
        QsLogDest.h
        QsLogDestConsole.cpp
        QsLogDestConsole.h
        QsLogDestFile.cpp
        QsLogDestFile.h
        QsLogDestFunctor.cpp
        QsLogDestFunctor.h
        QsLogDisableForThisFile.h
        QsLogLevel.h
        ${TS_FILES}

    )

    target_link_libraries(Qlog
        PUBLIC
            Qt5::Core
            Qt5::Widgets
            Qt5::Sql
    )

    include(GNUInstallDirs)

endif()
