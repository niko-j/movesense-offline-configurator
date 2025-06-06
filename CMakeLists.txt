cmake_minimum_required(VERSION 3.16)

project(movesense-offline-configurator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Bluetooth)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Bluetooth)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

set(MACOSX_BUNDLE_ICON_FILE movesense.icns)

# And the following tells CMake where to find and install the file itself.
set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/movesense.icns")
set_source_files_properties(${app_icon_macos} PROPERTIES
       MACOSX_PACKAGE_LOCATION "Resources")

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(movesense-offline-configurator
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${app_icon_macos}

        sensor.h sensor.cpp
        scanner.h scanner.cpp
        sessionlogdialog.h sessionlogdialog.cpp sessionlogdialog.ui
        protocol/packets/CommandPacket.cpp protocol/packets/CommandPacket.hpp protocol/packets/DataPacket.cpp protocol/packets/DataPacket.hpp protocol/packets/HandshakePacket.cpp protocol/packets/HandshakePacket.hpp protocol/packets/LogListPacket.cpp protocol/packets/LogListPacket.hpp protocol/packets/OfflineConfigPacket.cpp protocol/packets/OfflineConfigPacket.hpp protocol/packets/StatusPacket.cpp protocol/packets/StatusPacket.hpp protocol/packets/TimePacket.cpp protocol/packets/TimePacket.hpp protocol/types/OfflineConfig.hpp protocol/types/Packet.cpp protocol/types/Packet.hpp protocol/utils/Buffers.cpp protocol/utils/Buffers.hpp protocol/Protocol.hpp protocol/ProtocolConstants.hpp protocol/ProtocolPackets.hpp
        protocol/packets/CommandPacket.cpp protocol/packets/CommandPacket.hpp protocol/packets/DataPacket.cpp protocol/packets/DataPacket.hpp protocol/packets/DebugMessagePacket.cpp protocol/packets/DebugMessagePacket.hpp protocol/packets/HandshakePacket.cpp protocol/packets/HandshakePacket.hpp protocol/packets/LogListPacket.cpp protocol/packets/LogListPacket.hpp protocol/packets/OfflineConfigPacket.cpp protocol/packets/OfflineConfigPacket.hpp protocol/packets/StatusPacket.cpp protocol/packets/StatusPacket.hpp protocol/packets/TimePacket.cpp protocol/packets/TimePacket.hpp protocol/types/OfflineConfig.hpp protocol/types/Packet.cpp protocol/types/Packet.hpp protocol/utils/Buffers.cpp protocol/utils/Buffers.hpp protocol/Protocol.hpp protocol/ProtocolConstants.hpp protocol/ProtocolPackets.hpp

        logstreamview.h logstreamview.cpp logstreamview.ui
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET movesense-offline-configurator APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(movesense-offline-configurator SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(movesense-offline-configurator
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(movesense-offline-configurator PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Bluetooth
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.movesense-offline-configurator)
endif()
set_target_properties(movesense-offline-configurator PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS movesense-offline-configurator
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(movesense-offline-configurator)
endif()
