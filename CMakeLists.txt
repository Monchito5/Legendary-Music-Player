cmake_minimum_required(VERSION 3.16)

project(Legendary-Music-Player VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Encuentra los paquetes Qt necesarios
find_package(Qt6 COMPONENTS Widgets Multimedia REQUIRED)

set(PROJECT_SOURCES
      main.cpp
      mainwindow.cpp
      mainwindow.h
      mainwindow.ui
      playercontrols.cpp
      playercontrols.h
      playercontrols.ui
      playlistwidget.cpp
      playlistwidget.h
      playlistwidget.ui
      settingsdialog.cpp
      settingsdialog.h
      settingsdialog.ui
      visualizer.cpp
      visualizer.h
      visualizer.ui
      resources.qrc
)
if(${QT_VERSION_MAJOR})
    qt_add_executable(Legendary_Music_Player
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Legendary-Music-Player APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Legendary-Music-Player SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Legendary-Music-Player
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Legendary-Music-Player PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::Multimedia
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION})
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Legendary_Music_Player)
endif()
set_target_properties(Legendary-Music-Player PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Legendary-Music-Player
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Legendary_Music_Player)
endif()
