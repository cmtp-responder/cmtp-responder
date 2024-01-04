CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(cmtp-responder C)

include(GNUInstallDirs)

OPTION(BUILD_DESCRIPTORS "Build USB descriptors binary blobs" OFF)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/entity)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/transport)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/util)

SET(CONFDIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/cmtp-responder")
SET(CONFFILE "cmtp-responder.conf")
SET(CONFPATH "${CONFDIR}/${CONFFILE}")
ADD_DEFINITIONS("-DMTP_CONFIG_FILE_PATH=\"${CONFPATH}\"")
string(LENGTH "${CMAKE_SOURCE_DIR}/" SRC_PATH_LEN)
add_definitions("-DSRC_PATH_LEN=${SRC_PATH_LEN}")

SET(PKG_MODULES
	glib-2.0
	libsystemd
)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED ${PKG_MODULES})

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wall -Werror=implicit-function-declaration")
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -fexceptions -fvisibility=hidden -fprofile-arcs -ftest-coverage")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIE")
SET(CMAKE_EXE_LINKER_FLAGS " -Wl,--as-needed -pie -Wl,--hash-style=both")

ADD_SUBDIRECTORY(src/util)
ADD_SUBDIRECTORY(src/transport)
ADD_SUBDIRECTORY(src/entity)

ADD_EXECUTABLE(${PROJECT_NAME} src/mtp_cmd_handler.c src/mtp_cmd_handler_util.c src/mtp_event_handler.c src/mtp_init.c src/mtp_inoti_handler.c src/ptp_container.c)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
	entity
	transport
	util
	${pkgs_LDFLAGS}
	pthread
	rt
)

INSTALL(FILES ${CONFFILE} DESTINATION ${CONFDIR})

INSTALL(TARGETS ${PROJECT_NAME} RUNTIME)

IF (BUILD_DESCRIPTORS)
	ADD_EXECUTABLE(extract_descs_strs ${CMAKE_SOURCE_DIR}/src/extract_descs/mtp_extract_descs_strs.c
		${CMAKE_SOURCE_DIR}/src/transport/mtp_descs_strings.c)
	ADD_CUSTOM_COMMAND(OUTPUT descs strs
		COMMAND ./extract_descs_strs
		DEPENDS extract_descs_strs)
	ADD_CUSTOM_TARGET(descs_strs ALL DEPENDS descs strs)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/descs DESTINATION ${CONFDIR})
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/strs DESTINATION ${CONFDIR})
ENDIF ()