cmake_minimum_required(VERSION 3.21)
project(zcrazy_android LANGUAGES CXX)

# Enable Qt's meta-object code generation and resource handling
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Network)

# Note: Qt policy toggles (QTP0001/QTP0004) are available in newer Qt versions.
# Guard them to stay compatible with Qt 6.5.x Android kits.
if (COMMAND qt_policy)
    # QTP0001 exists in Qt >= 6.4, QTP0004 in newer versions; ignore if unknown
    execute_process(COMMAND ${CMAKE_COMMAND} -E echo "Qt policies guarded for compatibility")
endif()

qt_add_executable(zcrazy_android
    src/main.cpp
    src/CommandClient.cpp
    src/CommandClient.h
    src/network/UdpSender.cpp
    src/network/UdpSender.h
    src/network/StatusReceiver.cpp
    src/network/StatusReceiver.h
    src/network/RobotStatusModel.cpp
    src/network/RobotStatusModel.h
)

qt_add_qml_module(zcrazy_android
    URI ZCrazy
    VERSION 1.0
    NO_PLUGIN
    QML_FILES
        qml/Main.qml
        qml/MainSimple.qml
        qml/ControlPanel.qml
        qml/UI.qml
        qml/ZGroupBox.qml
        qml/ZText.qml
        qml/InfoViewer.qml
)

target_include_directories(zcrazy_android PRIVATE src src/network src/android)

target_link_libraries(zcrazy_android PRIVATE Qt6::Quick Qt6::Network)

# Android-specific sources (MulticastLock via JNI)
if(ANDROID)
    target_sources(zcrazy_android PRIVATE
        src/android/AndroidMulticastLock.cpp
        src/android/AndroidMulticastLock.h
    )
endif()

# Android packaging: merge files from the local android/ directory (no custom manifest),
# so we can override Gradle properties (e.g., compileSdk/targetSdk) without breaking manifest.
# Only merge gradle properties from a dedicated folder to avoid pulling in the legacy AndroidManifest.xml.
# Use default Qt Android packaging. If custom Gradle files are needed, ensure the directory contains only Gradle configs and valid manifest per Qt template.
# set_property(TARGET zcrazy_android PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android_pkg")

