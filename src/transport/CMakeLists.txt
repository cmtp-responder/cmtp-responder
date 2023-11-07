SET( TRANSPORT_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_descs_strings.c
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_transport.c
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_usb_driver.c
	)

add_library(transport STATIC ${TRANSPORT_SRC} )

