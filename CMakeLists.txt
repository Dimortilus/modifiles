cmake_minimum_required(VERSION 3.19)
project(ModiFiles LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Concurrent)

qt_standard_project_setup()

qt_add_executable(ModiFiles
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
	utils.cpp
	utils.h
)

target_link_libraries(ModiFiles
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Concurrent
)

include(GNUInstallDirs)

install(TARGETS ModiFiles
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ModiFiles
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
