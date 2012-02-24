#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODOsgUtils.cmake,v 1.6 2012-02-24 12:23:46 cvsbert Exp $
#_______________________________________________________________________________


IF ( (NOT DEFINED OSG_DIR) OR OSG_DIR STREQUAL "" )
    SET(OD_OSGDIR_ENV $ENV{OD_OSGDIR})

    IF(OD_OSGDIR_ENV)
        SET(OSG_DIR ${OD_OSGDIR_ENV} CACHE PATH "OSG Location" FORCE )
        MESSAGE( STATUS "Detecting OSG location: ${OSG_DIR}" )
    ENDIF()
ENDIF()


SET(OSGGEO_DIR ${OSG_DIR})

LIST(APPEND CMAKE_MODULE_PATH ${OSGGEO_DIR}/share/CMakeModules )

#SET DEBUG POSTFIX
SET (OLD_CMAKE_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )
SET (CMAKE_DEBUG_POSTFIX d)

FIND_PACKAGE(OSG)
FIND_PACKAGE(osgGeo)

#RESTORE DEBUG POSTFIX
SET (CMAKE_DEBUG_POSTFIX ${OLD_CMAKE_DEBUG_POSTFIX} )

IF ( (NOT DEFINED OSG_FOUND) OR (NOT DEFINED OSGGEO_FOUND) )
    SET(OSG_DIR "" CACHE PATH "OSG location" FORCE )
    MESSAGE( FATAL_ERROR "OSG_DIR not set")
ENDIF()


MACRO(OD_SETUP_OSG)
    IF(OD_USEOSG)

    #Prepare for crappy OSG-code ;-<
    IF( NOT DEFINED WIN32 )
	ADD_DEFINITIONS("-Wno-shadow -Wno-overloaded-virtual")
    ENDIF()
        LIST(APPEND OD_MODULE_INCLUDEPATH
		${OSGGEO_INCLUDE_DIR}
		${OSG_INCLUDE_DIR} )
	SET(OSGMODULES
		OSG
		OSGDB
		OSGGA
		OSGUTIL
		OSGQT
		OSGWIDGET
		OSGVIEWER
		OSGVOLUME
		OPENTHREADS
		OSGGEO )

	FOREACH( OSGMODULE ${OSGMODULES} )
	    IF ( ${OSGMODULE}_LIBRARY_DEBUG )
		LIST(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY_DEBUG} )
	    ELSE()
		IF ( NOT ${OSGMODULE}_LIBRARY )
		    MESSAGE(FATAL_ERROR "${OSGMODULE}_LIBRARY is missing")
		ENDIF()
		LIST(APPEND OD_OSG_LIBS ${${OSGMODULE}_LIBRARY} )
	    ENDIF()
	ENDFOREACH()
    ENDIF()

    LIST(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_OSG_LIBS} )

ENDMACRO(OD_SETUP_OSG)
