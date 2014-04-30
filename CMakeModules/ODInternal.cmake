#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

#Configure odversion.h
configure_file ( ${CMAKE_SOURCE_DIR}/include/Basic/odversion.h.in
		 ${CMAKE_BINARY_DIR}/include/Basic/odversion.h )

if ( NOT (CMAKE_BUILD_DIR STREQUAL CMAKE_SOURCE_DIR ) )
    if ( UNIX )
	    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                            ${CMAKE_SOURCE_DIR}/relinfo
			    ${CMAKE_BINARY_DIR}/relinfo )
    else()
	    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
                            ${CMAKE_SOURCE_DIR}/relinfo
			    ${CMAKE_BINARY_DIR}/relinfo )
    endif()

    #Copy data as we generate stuff into the data directory, and that cannot
    #be in the source-dir
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
		    ${CMAKE_SOURCE_DIR}/data
		    ${CMAKE_BINARY_DIR}/data )
endif()

file(GLOB CMAKE_FILES CMakeModules/*.cmake )
file(GLOB TEMPLATE_FILES CMakeModules/templates/*.in )
set( CMAKE_FILES ${CMAKE_FILES} ${TEMPLATE_FILES} )
OD_ADD_SOURCE_FILES( ${CMAKE_FILES} )

#Install cmake things.
install ( FILES ${CMAKE_BINARY_DIR}/CMakeModules/FindOpendTect.cmake
	  DESTINATION CMakeModules )
install ( DIRECTORY CMakeModules DESTINATION .
	  PATTERN ".svn" EXCLUDE
	  PATTERN "*.swp" EXCLUDE
	  PATTERN "*.cmake~" EXCLUDE
	  PATTERN "sourcefiles*.txt*" EXCLUDE)

#install doc stuff
file( GLOB TUTHFILES plugins/Tut/*.h )
file( GLOB TUTCCFILES plugins/Tut/*.cc )
set( TUTFILES ${TUTHFILES} ${TUTCCFILES} plugins/Tut/CMakeLists.txt )
install( FILES ${TUTFILES} DESTINATION doc/Programmer/pluginexample/plugins/Tut )

file( GLOB UITUTHFILES plugins/uiTut/*.h )
file( GLOB UITUTCCFILES plugins/uiTut/*.cc )
set( UITUTFILES ${UITUTHFILES} ${UITUTCCFILES} plugins/uiTut/CMakeLists.txt )
install( FILES ${UITUTFILES} DESTINATION doc/Programmer/pluginexample/plugins/uiTut )
install( FILES doc/Programmer/pluginexample/CMakeLists.txt
	 DESTINATION doc/Programmer/pluginexample )

install( DIRECTORY doc/Programmer/batchprogexample
	 DESTINATION doc/Programmer
	 PATTERN ".svn" EXCLUDE )

install( DIRECTORY doc/Credits/base
	 DESTINATION doc/Credits
	 PATTERN ".svn" EXCLUDE )

install( FILES doc/User/base/WindowLinkTable.txt
	 DESTINATION doc/User/base )
install( FILES doc/User/base/.mnuinfo
	 DESTINATION doc/User/base )

OD_CURRENT_MONTH( MONTH )
OD_CURRENT_YEAR( YEAR )

configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/about.html.in
		${CMAKE_BINARY_DIR}/doc/about.html @ONLY )
configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/license.txt.in
		${CMAKE_BINARY_DIR}/CMakeModules/license.txt @ONLY )

file( GLOB FLEXNETFILES doc/*.html )
foreach( FLEXNETFILE ${FLEXNETFILES} )
    install( FILES ${FLEXNETFILE} DESTINATION doc )
endforeach()

install( DIRECTORY doc/Scripts
	 DESTINATION doc
	 PATTERN ".svn" EXCLUDE )

#Install data
install ( DIRECTORY "data" DESTINATION .
	  PATTERN "install_files" EXCLUDE
	  PATTERN ".svn" EXCLUDE )

file( GLOB RELINFOFILES ${CMAKE_SOURCE_DIR}/relinfo/*.txt )
install ( FILES ${RELINFOFILES} DESTINATION relinfo )

install( FILES CMakeLists.txt DESTINATION . )

if( WIN32 )
    install( DIRECTORY bin/win32/rsm
	     DESTINATION .
	     PATTERN ".svn" EXCLUDE )
endif()

file( GLOB TEXTFILES ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/*.txt )
if( UNIX OR APPLE )
    file( GLOB PROGRAMS ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/* )
    list( REMOVE_ITEM PROGRAMS
	  ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/.svn )
    list( REMOVE_ITEM PROGRAMS
	  ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/makeself )
    foreach( TEXTFILE ${TEXTFILES} )
        list( REMOVE_ITEM PROGRAMS ${TEXTFILE} )
    endforeach()

    install ( PROGRAMS ${PROGRAMS} DESTINATION . )
endif()
install ( FILES ${TEXTFILES} DESTINATION . )

if( APPLE )
    install( DIRECTORY data/install_files/macscripts/Contents
	     DESTINATION .
	     PATTERN ".svn" EXCLUDE )
    OD_CURRENT_YEAR( YEAR )
    set( BUNDLEEXEC start_dtect )
    set( BUNDLEICON "od.icns" )
    set( BUNDLEID "com.dgbes.opendtect" )
    set ( INFOFILE CMakeModules/Info.plist )
    configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/Info.plist.in
		    ${CMAKE_BINARY_DIR}/${INFOFILE} @ONLY )
    install( FILES ${CMAKE_BINARY_DIR}/${INFOFILE} DESTINATION "Contents" )
endif( APPLE )

set( QJPEG ${QT_QJPEG_PLUGIN_RELEASE} )
set( LMHOSTID lmhostid )
if( WIN32 )
    if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
	set( QJPEG ${QTDIR}/plugins/imageformats/qjpegd4.dll )
    elseif ( ${CMAKE_BUILD_TYPE} STREQUAL "Release" )
	set( QJPEG ${QTDIR}/plugins/imageformats/qjpeg4.dll )
    endif()
    if( ${OD_PLFSUBDIR} STREQUAL "win32" )
	set( MSVCPATH "C:/Program\ Files \(x86\)/Microsoft\ Visual\ Studio\ 10.0/VC/redist/x86/Microsoft.VC100.CRT" )
    elseif( ${OD_PLFSUBDIR} STREQUAL "win64" )
	set( MSVCPATH "C:/Program\ Files \(x86\)/Microsoft\ Visual\ Studio\ 10.0/VC/redist/x64/Microsoft.VC100.CRT" )
    endif()
    install( DIRECTORY ${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_DEBUG}
	    DESTINATION bin/${OD_PLFSUBDIR}
	    CONFIGURATIONS Debug
	    FILES_MATCHING
	    PATTERN *.pdb
	)
    install( DIRECTORY ${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_DEBUG}
	    DESTINATION bin/${OD_PLFSUBDIR}
	    CONFIGURATIONS Debug
	    FILES_MATCHING
	    PATTERN *.lib
	)
    install( DIRECTORY ${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_RELEASE}
	    DESTINATION bin/${OD_PLFSUBDIR}
	    CONFIGURATIONS Release
	    FILES_MATCHING
	    PATTERN *.lib
	)
    set( LMHOSTID "lmhostid.exe" )
endif()

install( PROGRAMS ${QJPEG} DESTINATION imageformats )
if ( WIN32 )
    install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	     DESTINATION ${OD_EXEC_INSTALL_PATH_RELEASE}
	     CONFIGURATIONS Release )
    install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	     DESTINATION ${OD_EXEC_INSTALL_PATH_DEBUG}
	     CONFIGURATIONS Debug )
else()
    install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	     DESTINATION ${OD_EXEC_OUTPUT_RELPATH} )
endif()

if( EXISTS ${MSVCPATH} )
    file( GLOB MSVCDLLS ${MSVCPATH}/*.dll )
    foreach( DLL ${MSVCDLLS} )
	install( FILES ${DLL} DESTINATION ${OD_EXEC_INSTALL_PATH_RELEASE} CONFIGURATIONS Release )
	get_filename_component( FILENAME ${DLL} NAME )
	list( APPEND OD_THIRD_PARTY_LIBS ${FILENAME} )
    endforeach()
endif()

FILE( GLOB SCRIPTS ${CMAKE_SOURCE_DIR}/bin/od_* )
install( PROGRAMS ${SCRIPTS} DESTINATION bin )
install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/mksethdir DESTINATION bin )
install( FILES ${CMAKE_SOURCE_DIR}/bin/macterm.in DESTINATION bin )

#Installing unix syatem libraries
if ( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
    if( ${OD_PLFSUBDIR} STREQUAL "lux64" )
	OD_INSTALL_SYSTEM_LIBRARY( /usr/lib64/libstdc++.so.6 Release )
	OD_INSTALL_SYSTEM_LIBRARY( /lib64/libgcc_s.so.1 Release )
    elseif( ${OD_PLFSUBDIR} STREQUAL "lux32" )
	OD_INSTALL_SYSTEM_LIBRARY( /usr/lib/libstdc++.so.6 Release )
	OD_INSTALL_SYSTEM_LIBRARY( /lib/libgcc_s.so.1 Release )
    endif()
endif()

add_custom_target( sources ${CMAKE_COMMAND}
		   -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}

		   -DBINARY_DIR=${CMAKE_BINARY_DIR}
		   ${CMAKE_SOURCE_DIR} 
		   -P ${CMAKE_SOURCE_DIR}/CMakeModules/ODInstallSources.cmake
		   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		   COMMENT "Installing sources" )

add_custom_target( docpackages ${CMAKE_COMMAND}
        -DOpendTect_VERSION_MAJOR=${OpendTect_VERSION_MAJOR}
        -DOpendTect_VERSION_MINOR=${OpendTect_VERSION_MINOR}
        -DOpendTect_VERSION_DETAIL=${OpendTect_VERSION_DETAIL}
        -DOpendTect_VERSION_PATCH=${OpendTect_VERSION_PATCH}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DOD_PLFSUBDIR=${OD_PLFSUBDIR}
        -DPSD=${PROJECT_SOURCE_DIR}
        -P ${PROJECT_SOURCE_DIR}/CMakeModules/packagescripts/ODMakeDocPackages.cmake
         COMMENT "Preparing doc packages" )
include ( ODSubversion )

