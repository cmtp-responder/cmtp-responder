SET( ENTITY_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_device.c
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_object.c
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_property.c
	${CMAKE_CURRENT_SOURCE_DIR}/mtp_store.c
	)

add_library(entity STATIC ${ENTITY_SRC} )

