SET( UTIL_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_support.c
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_msgq.c
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_fs.c
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_util.c
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_thread.c
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_list.c
	)

add_library(util STATIC ${UTIL_SRC} )

