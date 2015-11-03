CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(mtp-responder C)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/entity)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/transport)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/util)

AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/src SRCS)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/src/entity SRCS)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/src/transport SRCS)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/src/util SRCS)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED glib-2.0 capi-content-media-content
	capi-media-metadata-extractor vconf dlog tapi libprivilege-control capi-system-info)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wall -Werror-implicit-function-declaration")
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -fexceptions -fvisibility=hidden")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIE")
SET(CMAKE_EXE_LINKER_FLAGS " -Wl,--as-needed -pie -Wl,--hash-style=both")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} pthread rt gcrypt)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
INSTALL(FILES mtp-responder.conf DESTINATION /opt/var/lib/misc)
