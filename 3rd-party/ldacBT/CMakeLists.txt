cmake_minimum_required(VERSION 3.0)
project(libldac
        LANGUAGES C
        VERSION 2.0.2.3)

set(CMAKE_C_STANDARD 99)


# option(LDAC_SOFT_FLOAT "For device without FPU, turn on this option" ON)
# if(${LDAC_SOFT_FLOAT} STREQUAL ON)
#     set(LDAC_SOFT_FLOAT_DEFINE "-D_32BIT_FIXED_POINT")
# endif()

if("${INSTALL_LIBDIR}" STREQUAL "")
    set(INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

if("${INSTALL_INCLUDEDIR}" STREQUAL "")
    set(INSTALL_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include)
endif()

if("${INSTALL_PKGCONFIGDIR}" STREQUAL "")
    set(INSTALL_PKGCONFIGDIR ${INSTALL_LIBDIR}/pkgconfig)
endif()

if("${INSTALL_LDAC_INCLUDEDIR}" STREQUAL "")
    set(INSTALL_LDAC_INCLUDEDIR ${INSTALL_INCLUDEDIR}/ldac)
endif()

# set(LDAC_SOFT_FLOAT ON)


add_subdirectory(pkgconfig)

include_directories(libldac/inc)
include_directories(libldac/abr/inc)

# libldacBT_enc
add_library(ldacBT_enc SHARED
        libldac/src/ldaclib.c
        libldac/src/ldacBT.c
        )

target_link_libraries(ldacBT_enc m)
target_compile_definitions(ldacBT_enc PUBLIC ${LDAC_SOFT_FLOAT_DEFINE})
set_target_properties(ldacBT_enc PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER libldac/inc/ldacBT.h
        )

# libldacBT_abr
add_library(ldacBT_abr SHARED
        libldac/abr/src/ldacBT_abr.c
        )

target_link_libraries(ldacBT_abr ldacBT_enc)
set_target_properties(ldacBT_abr PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER libldac/abr/inc/ldacBT_abr.h
        )

install(TARGETS
                ldacBT_enc
                ldacBT_abr
        LIBRARY
                DESTINATION ${INSTALL_LIBDIR}
        PUBLIC_HEADER
                DESTINATION ${INSTALL_LDAC_INCLUDEDIR}
        )

