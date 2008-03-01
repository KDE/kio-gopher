
set(kio_man_PART_SRCS kio_gopher.cpp)

kde4_add_plugin(kio_gopher ${kio_man_PART_SRCS})

target_link_libraries(kio_gopher  ${KDE4_KIO_LIBS})

install(TARGETS kio_gopher  DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES gopher.protocol DESTINATION  ${SERVICES_INSTALL_DIR})

