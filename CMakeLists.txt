project(kio_gopher)

cmake_minimum_required(VERSION 2.8.12)
set(QT_MIN_VERSION "5.2.0")

find_package(ECM 0.0.12 REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(ECMInstallIcons)
include(FeatureSummary)
include(GenerateExportHeader)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

find_package(Qt5 REQUIRED NO_MODULE
    COMPONENTS
    Core
)

find_package(KF5 REQUIRED
    COMPONENTS
    Codecs
    DocTools
    IconThemes
    I18n
    KIO
)

add_definitions(-DTRANSLATION_DOMAIN=\"kio_gopher\")
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

install(FILES gopher.protocol DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

