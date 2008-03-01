
if (NOT KDE4_FOUND)
	find_package(KDE4 REQUIRED)
	include(KDE4Defaults)
	add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
	include_directories(${KDE4_INCLUDES})
endif (NOT KDE4_FOUND)


set(kio_man_PART_SRCS kio_gopher.cpp)

kde4_add_plugin(kio_gopher ${kio_man_PART_SRCS})

target_link_libraries(kio_gopher  ${KDE4_KIO_LIBS})

install(TARGETS kio_gopher  DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES gopher.protocol DESTINATION  ${SERVICES_INSTALL_DIR})

