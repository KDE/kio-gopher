cmake_minimum_required(VERSION 3.0)

project(kio_gopher)

set(QT_MIN_VERSION 5.5.0)
set(KF5_MIN_VERSION 5.27.0)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMInstallIcons)
include(FeatureSummary)
include(GenerateExportHeader)

find_package(Qt${QT_MAJOR_VERSION} REQUIRED NO_MODULE
    COMPONENTS
    Core
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    COMPONENTS
    DocTools
    IconThemes
    I18n
    KIO
)

add_definitions(-DTRANSLATION_DOMAIN=\"kio5_gopher\")
add_definitions(
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050500
    -DQT_DEPRECATED_WARNINGS_SINCE=0x060000
    -DQT_NO_KEYWORDS
    -DQT_NO_URL_CAST_FROM_STRING
#     -DQT_NO_CAST_FROM_ASCII
#     -DQT_NO_CAST_TO_ASCII
#     -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_STRICT_ITERATORS
#     -DQT_USE_QSTRINGBUILDER
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
    -DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x051B00
    -DKF_DEPRECATED_WARNINGS_SINCE=0x060000
)

add_subdirectory(doc)

set(kio_gopher_PART_SRCS kio_gopher.cpp)

add_library(kio_gopher MODULE ${kio_gopher_PART_SRCS})

target_link_libraries(kio_gopher
    KF5::IconThemes
    KF5::I18n
    KF5::KIOCore
)

set_target_properties(kio_gopher PROPERTIES OUTPUT_NAME "gopher")
install(TARGETS kio_gopher DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
