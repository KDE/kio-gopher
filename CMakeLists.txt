cmake_minimum_required(VERSION 3.0)

project(kio_gopher)

set(QT_MIN_VERSION 5.5.0)
set(KF5_MIN_VERSION 5.99.0)
set(KDE_COMPILERSETTINGS_LEVEL 5.84.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMInstallIcons)
include(FeatureSummary)
include(GenerateExportHeader)
include(KDEClangFormat)
include(KDEGitCommitHooks)

find_package(Qt${QT_MAJOR_VERSION} REQUIRED NO_MODULE
    COMPONENTS
    Core
)

find_package(KF${QT_MAJOR_VERSION} ${KF5_MIN_VERSION} REQUIRED
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
    KF${QT_MAJOR_VERSION}::IconThemes
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::KIOCore
)

set_target_properties(kio_gopher PROPERTIES OUTPUT_NAME "gopher")
install(TARGETS kio_gopher DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kio)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
