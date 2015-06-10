CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(lbs-location C)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
#SET(LIBDIR "\${prefix}/lib")
#SET(INCLUDEDIR "\${prefix}/include")

# Set required packages
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED glib-2.0 gthread-2.0 gobject-2.0 gmodule-2.0
		capi-appfw-app-manager capi-appfw-package-manager pkgmgr-info
		privacy-manager-client dlog vconf json-glib-1.0 lbs-dbus bundle eventsystem)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS}  -Wl,-zdefs -fvisibility=hidden ")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wextra -fvisibility=hidden -fPIC")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-declarations -Wall -Wcast-align -Wno-sign-compare")

ADD_DEFINITIONS("-DFEATURE_DLOG_DEBUG")
ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS(" -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" ")

MESSAGE(${CMAKE_C_FLAGS})
MESSAGE(${CMAKE_EXE_LINKER_FLAGS})

# pkgconfig file
SET(PREFIX ${CMAKE_INSTALL_PREFIX})

CONFIGURE_FILE(lbs-location.pc.in lbs-location.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/lbs-location.pc DESTINATION ${LIBDIR}/pkgconfig)

#INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION /usr/share/license RENAME liblbs-dbus)

ADD_SUBDIRECTORY(location)
#ADD_SUBDIRECTORY(tests)
