#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       Nageswara
# Date:		August 2012		
#RCS:           $Id$

#TODO Change macro names to CAPITAL letters.

macro ( create_package PACKAGE_NAME )
    file( MAKE_DIRECTORY ${DESTINATION_DIR}/bin )

    file( MAKE_DIRECTORY ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
			 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
    file( MAKE_DIRECTORY ${DESTINATION_DIR}/plugins
			  ${DESTINATION_DIR}/plugins/${OD_PLFSUBDIR} )
    if( APPLE )
	set( MACBINDIR "Contents/MacOS" )
	file( MAKE_DIRECTORY ${DESTINATION_DIR}/Contents )
    endif()

    if( ${PACKAGE_NAME} STREQUAL "base" )
	if( APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/Contents/Resources/qt_menu.nib
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release/qt_menu.nib )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E chdir ${DESTINATION_DIR}/Contents
			     ln -s ../bin/mac/Release MacOS
			     RESULT_VARIABLE STATUS )
	    if( NOT ${STATUS} EQUAL "0" )
		message( FATAL_ERROR "Failed to create MacOS link" )
	    endif()
	endif()
    if( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
	copy_unix_systemlibs()
    endif()

        copy_thirdpartylibs()
        set( LIBLIST ${LIBLIST};${PLUGINS} )
    endif()

    set( COPYTODIR ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
    message( "Copying ${OD_PLFSUBDIR} libraries" )
    foreach( FILE ${LIBLIST} )
	if( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
		set(LIB "lib${FILE}.so")
	elseif( APPLE )
		set( LIB "lib${FILE}.dylib" )
	elseif( WIN32 )
		set( LIB "${FILE}.dll" )
	endif()

	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		    ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${LIB}
		    ${COPYTODIR} )
	file( GLOB ALOFILES ${CMAKE_INSTALL_PREFIX}/plugins/${OD_PLFSUBDIR}/*.${FILE}.alo )
	foreach( ALOFILE ${ALOFILES} )
	   execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${ALOFILE}
				    ${DESTINATION_DIR}/plugins/${OD_PLFSUBDIR} )
	endforeach()
    endforeach()

    if( ${PACKAGE_NAME} STREQUAL "dgbbase" )
#Inslall lm 
	foreach( SPECFILE ${SPECFILES} )
	     execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			      ${CMAKE_INSTALL_PREFIX}/${SPECFILE}
			      ${DESTINATION_DIR} )
	endforeach()

	execute_process( COMMAND ${CMAKE_COMMAND} -E
			 copy_directory ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/lm
			 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb )
	if( UNIX OR APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
                             ${CMAKE_INSTALL_PREFIX}/mk_flexlm_links
                             ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb )
	    execute_process( COMMAND
		${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb/mk_flexlm_links
		WORKING_DIRECTORY ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb
		RESULT_VARIABLE STATUS )
	    file( REMOVE_RECURSE ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb/mk_flexlm_links )
	    if( NOT ${STATUS} EQUAL "0" )
		message( "Failed to create license related links" )
	    endif()
	endif()	
    endif()

    if( WIN32 )
	if( ${PACKAGE_NAME} STREQUAL "base" )
		set( EXECLIST "${EXECLIST};${WINEXECLIST}" )
	endif()
    endif()

    message( "Copying ${OD_PLFSUBDIR} executables" )
    foreach( EXE ${EXECLIST} )
	if( WIN32 )
		set( EXE "${EXE}.exe" )
	endif()

	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		    ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${EXE} 
		    ${COPYTODIR} )
    endforeach()

    if( ${PACKAGE_NAME} STREQUAL "base" )
	foreach( SPECFILE ${SPECFILES} )
	     execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			      ${CMAKE_INSTALL_PREFIX}/${SPECFILE}
			      ${DESTINATION_DIR} )
	endforeach()
	foreach( FILES ${ODSCRIPTS} )
	     file( GLOB SCRIPTS ${CMAKE_INSTALL_PREFIX}/bin/${FILES} )
	     foreach( SCRIPT ${SCRIPTS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${SCRIPT}
					 ${DESTINATION_DIR}/bin )
	     endforeach()
	endforeach()

	if( WIN32 )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				     ${CMAKE_INSTALL_PREFIX}/rsm
				     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/rsm )
	endif()
    endif()

    zippackage( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
    message( "DONE" )
endmacro( create_package )


macro( copy_thirdpartylibs )
    set( COPYTODIR ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
    message( "Copying ${OD_PLFSUBDIR} thirdparty libraries" )
    set( FROMDIR ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release )
    foreach( LIB ${OD_THIRD_PARTY_LIBS} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${FROMDIR}/${LIB} ${COPYTODIR} )
    endforeach()

    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${CMAKE_INSTALL_PREFIX}/imageformats
		     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release/imageformats )
endmacro( copy_thirdpartylibs )

macro( PREPARE_WIN_THIRDPARTY_DEBUGLIST DEBUGFILELIST)
    if( WIN32 )
	foreach( THIRDPARTYDLL ${OD_THIRD_PARTY_LIBS} )
	    get_filename_component(FILENM ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${THIRDPARTYDLL} NAME_WE)
	    string( FIND ${FILENM} "4" ISQTFILE )
	    string( FIND ${FILENM} "osg" ISOSGFILE )
	    string( FIND ${FILENM} "OpenThreads" OSGOTFILE )
	if ( NOT "${ISQTFILE}" EQUAL -1 ) #  File is Qt
	    string(REGEX REPLACE "4" "d4" QTDEBUGFILENAME ${FILENM} )
	    list(APPEND ${DEBUGFILELIST} ${QTDEBUGFILENAME}.dll )
	elseif ( (NOT "${ISOSGFILE}" EQUAL -1) OR (NOT "${OSGOTFILE}" EQUAL -1) ) # File is osg
	    string(REGEX REPLACE "${FILENM}" "${FILENM}d" OSGDEBUGFILENAME ${FILENM} )
	    list(APPEND ${DEBUGFILELIST} ${OSGDEBUGFILENAME}.dll )
	endif()
	endforeach()
	list(REMOVE_DUPLICATES ${DEBUGFILELIST} )
    endif()
endmacro()

macro( copy_unix_systemlibs )
    message( "Copying ${OD_PLFSUBDIR} system libraries" )
    set( COPYTODIR ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
    set( FROMDIR ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release )
    if( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
	foreach( SYSLIB ${SYSTEMLIBS} )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${FROMDIR}/${SYSLIB} ${COPYTODIR} )
	endforeach()
    endif()
endmacro()

macro( create_basepackages PACKAGE_NAME )
   if( EXISTS ${DESTINATION_DIR}/Contents )
	file( REMOVE_RECURSE ${DESTINATION_DIR}/Contents )
   endif()
   if( NOT EXISTS ${DESTINATION_DIR}/doc )
	file( MAKE_DIRECTORY ${DESTINATION_DIR}/doc ${DESTINATION_DIR}/doc/User
			     ${DESTINATION_DIR}/doc/ReleaseInfo)
   endif()

    if( ${PACKAGE_NAME} STREQUAL "basedata" )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_INSTALL_PREFIX}/doc/about.html
			${DESTINATION_DIR}/doc )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_INSTALL_PREFIX}/doc/system_requirements.html
			${DESTINATION_DIR}/doc )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/relinfo/RELEASE.txt
				${DESTINATION_DIR}/doc/ReleaseInfo )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/relinfo/RELEASEINFO.txt
				${DESTINATION_DIR}/doc/ReleaseInfo )
	foreach( LIBS ${LIBLIST} )
	    file( GLOB DATAFILES ${CMAKE_INSTALL_PREFIX}/data/${LIBS} )
	    foreach( DATA ${DATAFILES} )
    #TODO if possible copy files instead of INSTALL
		  file( INSTALL DESTINATION ${DESTINATION_DIR}/data
				TYPE DIRECTORY FILES ${DATA}
				REGEX ".svn" EXCLUDE )
	    endforeach()
	endforeach()
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/relinfo/README.txt
			 ${DESTINATION_DIR}/relinfo )
#install WindowLinkTable.txt
       file( MAKE_DIRECTORY ${DESTINATION_DIR}/doc/User/base )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/doc/User/base/WindowLinkTable.txt
				${DESTINATION_DIR}/doc/User/base/WindowLinkTable.txt )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/doc/User/base/.mnuinfo
				${DESTINATION_DIR}/doc/User/base/.mnuinfo )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy 
			        ${CMAKE_INSTALL_PREFIX}/doc/od_LinkFileTable.txt
				${DESTINATION_DIR}/doc/User/base/LinkFileTable.txt )
   endif()
   if( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/relinfo/RELEASE.dgb.txt
				${DESTINATION_DIR}/doc/ReleaseInfo )
       foreach( LIB ${LIBLIST} )
	  if( IS_DIRECTORY "${CMAKE_INSTALL_PREFIX}/data/${LIB}" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/data/${LIB}
			     ${DESTINATION_DIR}/data/${LIB} )
	  else()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${CMAKE_INSTALL_PREFIX}/data/${LIB}
			     ${DESTINATION_DIR}/data/${LIB} )
	  endif()
       endforeach()
#install WindowLinkTable.txt
       file( MAKE_DIRECTORY ${DESTINATION_DIR}/doc/User/dgb )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			        ${CMAKE_INSTALL_PREFIX}/doc/dgb_WindowLinkTable.txt
				${DESTINATION_DIR}/doc/User/dgb/WindowLinkTable.txt )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			        ${CMAKE_INSTALL_PREFIX}/doc/dgb_.mnuinfo
				${DESTINATION_DIR}/doc/User/dgb/.mnuinfo )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy 
				${CMAKE_INSTALL_PREFIX}/doc/dgb_LinkFileTable.txt
				${DESTINATION_DIR}/doc/User/dgb/LinkFileTable.txt )
   endif()

    zippackage( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( create_basepackages )


macro( init_destinationdir  PACKAGE_NAME )
    set ( PACKAGE_FILENAME ${PACKAGE_NAME} )
    set( PACKAGE_FILENAME "${PACKAGE_FILENAME}_${OD_PLFSUBDIR}.zip" )
    set( VER_FILENAME "${PACKAGE_NAME}_${OD_PLFSUBDIR}" )
    if( ${PACKAGE_NAME} STREQUAL "basedata" )
        set( VER_FILENAME "basedata" )
        set( PACKAGE_FILENAME "basedata.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "basedata_mac.zip" )
	    set( VER_FILENAME "basedata_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
        set( VER_FILENAME "dgbbasedata" )
        set( PACKAGE_FILENAME "dgbbasedata.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "dgbbasedata_mac.zip" )
	    set( VER_FILENAME "dgbbasedata_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "doc" )
        set( VER_FILENAME "doc" )
        set( PACKAGE_FILENAME "doc.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "doc_mac.zip" )
	    set( VER_FILENAME "doc_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
        set( VER_FILENAME "dgbdoc" )
        set( PACKAGE_FILENAME "dgbdoc.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "dgbdoc_mac.zip" )
	    set( VER_FILENAME "dgbdoc_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "classdoc" )
	set( VER_FILENAME "classdoc" )
	set( PACKAGE_FILENAME "classdoc.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "classdoc_mac.zip" )
	    set( VER_FILENAME "classdoc_mac" )
	endif()
    endif()

    if( NOT EXISTS ${SOURCE_DIR}/packages )
	    file( MAKE_DIRECTORY ${SOURCE_DIR}/packages )
    endif()

    set( PACKAGE_DIR ${SOURCE_DIR}/packages )
    if( EXISTS ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
	file( REMOVE_RECURSE ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
    endif()
    set( REL_DIR "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_DETAIL}" )
    set( FULLVER_NAME "${REL_DIR}${OpendTect_VERSION_PATCH}" )
    if( APPLE )
	set( REL_DIR "OpendTect\ ${REL_DIR}.app" )
    endif()

    set( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
    if( EXISTS ${DESTINATION_DIR} )
	file( REMOVE_RECURSE ${DESTINATION_DIR} )
    endif()

    file( MAKE_DIRECTORY ${DESTINATION_DIR} ${DESTINATION_DIR}/relinfo )
    file( WRITE ${DESTINATION_DIR}/relinfo/ver.${VER_FILENAME}.txt ${FULLVER_NAME} )
    file( APPEND ${DESTINATION_DIR}/relinfo/ver.${VER_FILENAME}.txt "\n" )

    if( APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/Contents
			     ${DESTINATION_DIR}/Contents )
    endif()

    message( "Preparing package ${VER_FILENAME}.zip ......" )
endmacro( init_destinationdir )


macro( create_develpackages )
    file( MAKE_DIRECTORY ${DESTINATION_DIR}/doc
			 ${DESTINATION_DIR}/doc/Programmer)
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
	    	     ${SOURCE_DIR}/CMakeLists.txt ${DESTINATION_DIR} )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${CMAKE_INSTALL_PREFIX}/doc/Programmer/batchprogexample
		     ${DESTINATION_DIR}/doc/Programmer/batchprogexample )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${CMAKE_INSTALL_PREFIX}/doc/Programmer/pluginexample
		     ${DESTINATION_DIR}/doc/Programmer/pluginexample )
    file( GLOB HTMLFILES ${BINARY_DIR}/doc/Programmer/*.html )
    foreach( HTMLFILE ${HTMLFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${HTMLFILE} ${DESTINATION_DIR}/doc/Programmer )
    endforeach()
    file( GLOB PNGFILES ${SOURCE_DIR}/doc/Programmer/*.png )
    foreach( PNGFILE ${PNGFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${PNGFILE} ${DESTINATION_DIR}/doc/Programmer )
    endforeach()

    foreach( DIR CMakeModules include src plugins spec )
	message( "Copying ${DIR} files" )
	if( "${DIR}" STREQUAL "plugins" )
	    foreach( ODPLUGIN ${ODPLUGINS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${CMAKE_INSTALL_PREFIX}/plugins/${ODPLUGIN}
				 ${DESTINATION_DIR}/plugins/${ODPLUGIN} )
	    endforeach()
	else()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/${DIR}
			     ${DESTINATION_DIR}/${DIR} )
	endif()
    endforeach()

    if( WIN32 )
	file( MAKE_DIRECTORY ${DESTINATION_DIR}/bin
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${SOURCE_DIR}/bin/od_cr_dev_env.bat
			 ${DESTINATION_DIR}/bin )

	set( DEVELLIBS ${ODLIBLIST} ${ODPLUGINS} ${SPECSOURCES} )
	#Copying dll, pdb and lib files.
	foreach( DLIB ${DEVELLIBS} )
	    file( GLOB FILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${DLIB}.* )
	    string( TOLOWER ${DLIB} PDBLIB )
	    file( GLOB PDBFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${PDBLIB}.* )
	    set( FILES ${FILES} ${PDBFILES} )
	    foreach( FIL ${FILES} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			${FIL} ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug )
	    endforeach()

	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${DLIB}.lib
		${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
	endforeach()
	#Copying executables and pdb files
	foreach( EXELIB ${EXECLIST} )
	    file( GLOB EXEFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${EXELIB}.* )
	    string( TOLOWER ${EXELIB} EXEPDBLIB )
	    file( GLOB EXEPDBFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${EXEPDBLIB}.* )
	    set( EXEILES ${EXEFILES} ${EXEPDBFILES} )
	    foreach( ELIB ${EXEILES} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			${ELIB} ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug )
	    endforeach()
	endforeach()

	#Copying third party debug libraries
	PREPARE_WIN_THIRDPARTY_DEBUGLIST( DEBUGLIST )
	foreach( TLIB ${DEBUGLIST} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${TLIB}
				${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug )
	endforeach()
    endif()

    zippackage( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( create_develpackages )


macro( download_packages  )
    message( "downloading doc pkgs" )
#    set( DOCNAMES appman workflows user dgb )
    set( DOCNAMES dgb )
    foreach( DOCNAME ${DOCNAMES} )
	set( url "http://intranet/documentations/devel" )
	set( url "${url}/${DOCNAME}doc.zip" )
	set( DIRNAME ${DOCNAME} )
        if( ${DOCNAME} STREQUAL "appman" )
	    set( DIRNAME SysAdm )
	elseif( ${DOCNAME} STREQUAL "user")
	    set( DIRNAME base )
	endif()

	if( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME} )
	    file( REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME} )
	endif()

	file( DOWNLOAD ${url} "${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME}/${DOCNAME}doc.zip"
	      STATUS var
	      LOG log
	      SHOW_PROGRESS)
	if( NOT var EQUAL "0" )
	    message( "***${url} Download Failed***")
	else()
	    execute_process( COMMAND unzip -o -q
			     ${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME}/${DOCNAME}doc.zip
			     -d ${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME}
			     RESULT_VARIABLE STATUS )
	    file( REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/doc/${DIRNAME}/${DOCNAME}doc.zip )
	    if( NOT STATUS EQUAL "0" )
		message( "*** unzip Failed***")
	    endif()
	endif()
    endforeach()
endmacro( download_packages )

macro( create_docpackages PACKAGE_NAME )
    if( WIN32 )
	message( FATAL_ERROR "Documentation packages will create only on Linux ans Mac" )
    else()
	if( ${PACKAGE_NAME} STREQUAL "doc" )
	    if( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/base/LinkFileTable.txt )
		file( RENAME ${CMAKE_INSTALL_PREFIX}/doc/base/LinkFileTable.txt
			     ${CMAKE_INSTALL_PREFIX}/doc/od_LinkFileTable.txt )
	    endif()

	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/SysAdm
			     ${DESTINATION_DIR}/doc/SysAdm )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/Scripts
			     ${DESTINATION_DIR}/doc/Scripts )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/workflows
			     ${DESTINATION_DIR}/doc/User/workflows )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/base
			     ${DESTINATION_DIR}/doc/User/base )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/Credits/base
			     ${DESTINATION_DIR}/doc/Credits/base )
	elseif( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/Credits/dgb
			     ${DESTINATION_DIR}/doc/Credits/dgb )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/dgb
			     ${DESTINATION_DIR}/doc/User/dgb )
	    file( GLOB FILES ${CMAKE_INSTALL_PREFIX}/doc/flexnet* )
	    foreach( FIL ${FILES} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${FIL} ${DESTINATION_DIR}/doc )
	    endforeach()
	    if( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/dgb/LinkFileTable.txt )
		file( RENAME ${CMAKE_INSTALL_PREFIX}/doc/dgb/LinkFileTable.txt
			     ${CMAKE_INSTALL_PREFIX}/doc/dgb_LinkFileTable.txt )
	    endif()
	elseif( ${PACKAGE_NAME} STREQUAL "classdoc" )
	    if( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/Programmer/Generated )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${CMAKE_INSTALL_PREFIX}/doc/Programmer/Generated
				 ${DESTINATION_DIR}/doc/Programmer/Generated )
	    else()
		message( "Class doc not installed correctly. ${PACKAGE_FILENAME} is not self contained." )
	    endif()
	endif()
    endif()
    zippackage( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( create_docpackages )

macro( zippackage PACKAGE_FILENAME REL_DIR PACKAGE_DIR )
    if( WIN32 )
	message( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND ${SOURCE_DIR}/bin/win64/zip -r -q
			 	 "${PACKAGE_FILENAME}" ${REL_DIR}
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    else()
	message( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    endif()

    if( NOT ${STATUS} EQUAL "0" )
	message( FATAL_ERROR "Failed to create zip file ${PACKAGE_FILENAME}" )
   endif()
endmacro( zippackage )

#Genarate Symbols and then Strip the binaries
macro ( OD_GENERATE_BREAKPAD_SYMBOLS ALLLIBS EXECS)
if( UNIX )
    if ( NOT DEFINED BREAKPAD_DIR )
	message ( FATAL_EEROR "BREAKPAD_DIR not defined" )
    endif()
    if ( NOT EXISTS ${BREAKPAD_DIR} )
	message ( FATAL_EEROR "BREAKPAD_DIR: ${BREAKPAD_DIR} not found" )
    endif()

    if( NOT EXISTS ${CMAKE_INSTALL_PREFIX}/symbols )
	    file( MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/symbols )
    endif()

    set( LIBNAMES "" )
    foreach( LIB ${ALLLIBS} )
	set(SOLIB "lib${LIB}.so")
	set( LIBNAMES ${LIBNAMES} ${SOLIB} )
    endforeach()
    set (PLFSYM linux)

    set( ALLLIBSBINS ${LIBNAMES} ${EXECS} )
    foreach( FILENAME ${ALLLIBSBINS} )
	if ( EXISTS ${CMAKE_INSTALL_PREFIX}/symbols/${FILENAME} )
	    file( REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/symbols/${FILENAME} )
	endif()
	file( MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/symbols/${FILENAME} )

	set( SYMGENCMD ${BREAKPAD_DIR}/tools/${PLFSYM}/dump_syms/dump_syms )
	execute_process( COMMAND "${SYMGENCMD}" ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${FILENAME} OUTPUT_FILE ${CMAKE_INSTALL_PREFIX}/symbols/${FILENAME}.sym )
	file( STRINGS ${CMAKE_INSTALL_PREFIX}/symbols/${FILENAME}.sym STUFF LIMIT_COUNT 1 )
	string( REGEX REPLACE " " ";" NEWSTUFF ${STUFF} )
	set( WORDS ${NEWSTUFF} )
	list( GET WORDS 3 SYMADDRESS ) #assuming 4th word is symbol address
	file( MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/symbols/${FILENAME}/${SYMADDRESS} )
	file( RENAME ${CMAKE_INSTALL_PREFIX}/symbols/${FILENAME}.sym ${CMAKE_INSTALL_PREFIX}/symbols/${FILENAME}/${SYMADDRESS}/${FILENAME}.sym )
	execute_process( COMMAND strip ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${FILENAME} )
    endforeach()

endif()
endmacro() #OD_GENERATE_BREAKPAD_SYMBOLS

