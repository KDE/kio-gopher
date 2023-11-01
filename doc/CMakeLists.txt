if (QT_MAJOR_VERSION STREQUAL "5")
    set(DOCSUBDIR "kioslave5/gopher")
else()
    set(DOCSUBDIR "kioworker6/gopher")
endif()

kdoctools_create_handbook(index.docbook INSTALL_DESTINATION ${KDE_INSTALL_DOCBUNDLEDIR}/en SUBDIR ${DOCSUBDIR})

