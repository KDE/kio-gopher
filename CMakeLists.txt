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

find_package(Qt5 REQUIRED NO_MODULE
    COMPONENTS
    Core
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    COMPONENTS
    Codecs
    DocTools
    IconThemes
    I18n
    KIO
)

add_definitions(-DTRANSLATION_DOMAIN=\"kio5_gopher\")
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
   add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x060000)
endif()

add_subdirectory(doc)

set(kio_gopher_PART_SRCS kio_gopher.cpp)

add_library(kio_gopher MODULE ${kio_gopher_PART_SRCS})

target_link_libraries(kio_gopher
    KF5::Codecs
    KF5::IconThemes
    KF5::I18n
    KF5::KIOCore
)

set_target_properties(kio_gopher PROPERTIES OUTPUT_NAME "gopher")
install(TARGETS kio_gopher DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
