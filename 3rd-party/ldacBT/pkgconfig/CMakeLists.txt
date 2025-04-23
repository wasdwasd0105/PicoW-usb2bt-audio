set(PREFIX ${CMAKE_INSTALL_PREFIX})
set(LIBDIR ${INSTALL_LIBDIR})
set(INCLUDEDIR ${INSTALL_LDAC_INCLUDEDIR})
set(VERSION ${PROJECT_VERSION})

set(PC_VAR_PREFIX "\${prefix}")
set(PC_VAR_LIBDIR "\${libdir}")
set(PC_VAR_INCLUDEDIR "\${includedir}")

configure_file(
        ldacBT-enc.pc.in
        ldacBT-enc.pc
)
configure_file(
        ldacBT-abr.pc.in
        ldacBT-abr.pc
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ldacBT-enc.pc
        ${CMAKE_CURRENT_BINARY_DIR}/ldacBT-abr.pc
        DESTINATION
        ${INSTALL_PKGCONFIGDIR})
