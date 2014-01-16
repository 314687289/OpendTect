#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

set( QTDIR "" CACHE PATH "QT Location" )

macro(ADD_TO_LIST_IF_NEW LISTNAME ITEMNAME)
    list( FIND ${LISTNAME} "${ITEMNAME}" ITMINDEX )
    if ( ${ITMINDEX} EQUAL -1 )
	list(APPEND ${LISTNAME} "${ITEMNAME}")
    endif()
endmacro(ADD_TO_LIST_IF_NEW)

macro(OD_SETUP_QT)
    if ( (NOT DEFINED QTDIR) OR QTDIR STREQUAL "" )
	MESSAGE( FATAL_ERROR "QTDIR not set")
    endif()

    #Try to find Qt5
    list ( APPEND CMAKE_PREFIX_PATH ${QTDIR} )
    find_package( Qt5Core QUIET )
    if ( Qt5Core_FOUND )
	find_package( Qt5 REQUIRED ${OD_USEQT} )

	if( QT_MOC_HEADERS )
	    foreach( HEADER ${QT_MOC_HEADERS} )
		list( APPEND QT_MOC_INPUT
		      ${CMAKE_SOURCE_DIR}/src/${OD_MODULE_NAME}/${HEADER} )
	    endforeach()
	    QT5_WRAP_CPP( QT_MOC_OUTFILES ${QT_MOC_INPUT} )
	endif( QT_MOC_HEADERS )

	foreach ( QTMOD ${OD_USEQT} )
	    list( APPEND OD_MODULE_INCLUDESYSPATH ${Qt5${QTMOD}_INCLUDE_DIRS} )
	    list( APPEND OD_QT_LIBS ${Qt5${QTMOD}_LIBRARIES} )
	endforeach()

	list( REMOVE_DUPLICATES OD_QT_LIBS )
	list( REMOVE_DUPLICATES OD_MODULE_INCLUDESYSPATH )
	list( APPEND OD_MODULE_EXTERNAL_LIBS ${OD_QT_LIBS} )

	if ( WIN32 )
	    set ( CMAKE_CXX_FLAGS "/WD4481 ${CMAKE_CXX_FLAGS}" )
	endif( WIN32 )
    else() # Use Qt4

	set( ENV{QTDIR} ${QTDIR} )
	set ( QT_QMAKE_EXECUTABLE ${QTDIR}/bin/qmake${CMAKE_EXECUTABLE_SUFFIX} )
	find_package(Qt4 REQUIRED QtGui QtCore QtSql QtNetwork )

	include(${QT_USE_FILE})

	STRING( FIND "${OD_USEQT}" "Core" USE_QT_CORE )
	if( NOT "${USE_QT_CORE}" EQUAL -1 )
	list( APPEND OD_MODULE_INCLUDESYSPATH
		${QT_QTCORE_INCLUDE_DIR} ${QTDIR}/include )
	ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTCORE_LIBRARY}" )
	endif()

	STRING( FIND "${OD_USEQT}" "Network" USE_QT_NETWORK )
	if( NOT "${USE_QT_NETWORK}" EQUAL -1 )
	list( APPEND OD_MODULE_INCLUDESYSPATH
	    ${QT_QTNETWORK_INCLUDE_DIR} ${QTDIR}/include )
	ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTNETWORK_LIBRARY}" )
	endif()

	STRING( FIND "${OD_USEQT}" "Sql" USE_QT_SQL )
	if( NOT "${USE_QT_SQL}" EQUAL -1 )
	list(APPEND OD_MODULE_INCLUDESYSPATH
	    ${QT_QTSQL_INCLUDE_DIR}
	    ${QTDIR}/include )
	ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTSQL_LIBRARY}")
	endif()

	STRING( FIND "${OD_USEQT}" "Widgets" USE_QT_GUI )
	if( NOT "${USE_QT_GUI}" EQUAL -1 )
	list(APPEND OD_MODULE_INCLUDESYSPATH
	    ${QT_QTCORE_INCLUDE_DIR}
	    ${QT_QTGUI_INCLUDE_DIR} ${QTDIR}/include )
	ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTGUI_LIBRARY}")
	endif()

	STRING( FIND "${OD_USEQT}" "OpenGL" USE_QT_OPENGL )
	if( NOT "${USE_QT_OPENGL}" EQUAL -1 )
	list(APPEND OD_MODULE_INCLUDESYSPATH
	    ${QT_QTOPENGL_INCLUDE_DIR} ${QTDIR}/include )
	ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTOPENGL_LIBRARY}")
	endif()

	STRING( FIND "${OD_USEQT}" "Xml" USE_QT_XML )
	if( NOT "${USE_QT_XML}" EQUAL -1 )
	list(APPEND OD_MODULE_INCLUDESYSPATH
	    ${QT_QTXML_INCLUDE_DIR} ${QTDIR}/include )
	ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTXML_LIBRARY}")
	endif()

	list( REMOVE_DUPLICATES OD_MODULE_INCLUDESYSPATH )

	if( QT_MOC_HEADERS )
	foreach( HEADER ${QT_MOC_HEADERS} )
	    list(APPEND QT_MOC_INPUT
		${CMAKE_SOURCE_DIR}/src/${OD_MODULE_NAME}/${HEADER})
	endforeach()

	QT4_WRAP_CPP (QT_MOC_OUTFILES ${QT_MOC_INPUT})
	endif( QT_MOC_HEADERS )

	list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_QT_LIBS} )
	if ( OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
	    foreach( BUILD_TYPE Debug Release )
		set( QARGS ${QT_QTOPENGL_LIBRARY} ${QT_QTCORE_LIBRARY}
			   ${QT_QTNETWORK_LIBRARY} ${QT_QTSQL_LIBRARY}
			   ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY} )

		OD_FILTER_LIBRARIES( QARGS ${BUILD_TYPE} )
		unset( ARGS )

		foreach( QLIB ${QARGS} )
		    get_filename_component( QLIBNAME ${QLIB} NAME_WE )
		    if( UNIX OR APPLE )
			if( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
			    set( FILENM "${QLIBNAME}.so.${QT_VERSION_MAJOR}" )
			elseif( APPLE )
			    set( FILENM "${QLIBNAME}.${QT_VERSION_MAJOR}.dylib" )
			endif()
			OD_INSTALL_LIBRARY( ${QTDIR}/lib/${FILENM} ${BUILD_TYPE} )
		    elseif( WIN32 )
			OD_INSTALL_LIBRARY( ${QTDIR}/bin/${QLIBNAME}.dll ${BUILD_TYPE} )
			install( PROGRAMS ${QLIB}
				 DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${BUILD_TYPE}
				 CONFIGURATIONS ${BUILD_TYPE} )
		    endif()
		endforeach()
	    endforeach()

	endif()

	if ( WIN32 )
	    set ( CMAKE_CXX_FLAGS "/Zc:wchar_t- ${CMAKE_CXX_FLAGS}" )
	endif( WIN32 )

    endif()
endmacro( OD_SETUP_QT )


