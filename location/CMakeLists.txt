CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(location C)

SET(LIB_NAME "lbs-location")
SET(INC_DIR "include")
SET(MANAGER_DIR "manager")
SET(MODULE_DIR "module")

IF(FEATURE_PROFILE_TV)
	SET (PRIVACY_SRC "")
ELSE(FEATURE_PROFILE_TV)
	SET (PRIVACY_SRC "${MANAGER_DIR}/location-privacy.c")
ENDIF(FEATURE_PROFILE_TV)

IF(FEATURE_PROFILE_WEARABLE)
	MESSAGE("<<< Wearable Profile >>>")
ELSE(FEATURE_PROFILE_WEARABLE)
	MESSAGE("<<< Mobile Profile >>>")
	ADD_DEFINITIONS("-DTIZEN_PROFILE_MOBILE")
ENDIF(FEATURE_PROFILE_WEARABLE)

ADD_DEFINITIONS("-DLIBDIR=\"${LIBDIR}\"")

INCLUDE_DIRECTORIES(
	${INC_DIR}
	${MANAGER_DIR}
	${MODULE_DIR}
)

### Build ###
SET(SRCS
	${MANAGER_DIR}/location.c
	${MANAGER_DIR}/location-marshal.c
	${MANAGER_DIR}/location-ielement.c
	${MANAGER_DIR}/location-setting.c
	${MANAGER_DIR}/location-position.c
	${MANAGER_DIR}/location-batch.c
	${MANAGER_DIR}/location-velocity.c
	${MANAGER_DIR}/location-accuracy.c
	${MANAGER_DIR}/location-boundary.c
	${MANAGER_DIR}/location-satellite.c
	${MANAGER_DIR}/location-signaling-util.c
	${MANAGER_DIR}/location-common-util.c
	${MANAGER_DIR}/location-gps.c
	${MANAGER_DIR}/location-wps.c
	${MANAGER_DIR}/location-mock.c
	${MANAGER_DIR}/location-fused.c
	${PRIVACY_SRC}

	${MODULE_DIR}/module-internal.c
)

SET(SRCS_MOBILE
	${MANAGER_DIR}/location-hybrid-mobile.c
)

SET (INSTALL_HEADERS
	${INC_DIR}/location-types.h

	${MANAGER_DIR}/location.h
	${MANAGER_DIR}/location-position.h
	${MANAGER_DIR}/location-batch.h
	${MANAGER_DIR}/location-velocity.h
	${MANAGER_DIR}/location-accuracy.h
	${MANAGER_DIR}/location-boundary.h
	${MANAGER_DIR}/location-satellite.h

	${MODULE_DIR}/location-module.h
)

ADD_LIBRARY(${LIB_NAME} SHARED ${SRCS} ${SRCS_MOBILE})
TARGET_LINK_LIBRARIES(${LIB_NAME} ${pkgs_LDFLAGS} -lm)
SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES VERSION ${FULLVER} SOVERSION ${MAJORVER} CLEAN_DIRECT_OUTPUT 1)

IF (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${MANAGER_DIR}/location-marshal.c)
	MESSAGE ("")
	MESSAGE ("<<< Generating location-marshal.c >>>")

	EXECUTE_PROCESS(
		COMMAND glib-genmarshal --header --prefix=location ${CMAKE_CURRENT_SOURCE_DIR}/manager/location-marshal.list
		OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/manager/location-marshal.h
	)

	EXECUTE_PROCESS(
		COMMAND glib-genmarshal --body --prefix=location ${CMAKE_CURRENT_SOURCE_DIR}/manager/location-marshal.list
		OUTPUT_VARIABLE marshal_var
	)

	EXECUTE_PROCESS(
		COMMAND echo "#include \"location-marshal.h\" ${marshal_var}"
		OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/manager/location-marshal.c
	)

ENDIF (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${MANAGER_DIR}/location-marshal.c)

INSTALL(TARGETS ${LIB_NAME} DESTINATION ${LIBDIR} COMPONENT Runtime)
FOREACH(ITEM IN ITEMS ${INSTALL_HEADERS})
	MESSAGE ("${ITEM}")
	INSTALL(FILES ${ITEM} DESTINATION ${INCLUDEDIR}/${PROJECT_NAME})
ENDFOREACH()
