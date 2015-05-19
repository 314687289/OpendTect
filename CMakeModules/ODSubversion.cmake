#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

if ( EXISTS ${CMAKE_SOURCE_DIR}/.svn )
    set ( OD_FROM_SVN 1 )
endif()

# the FindSubversion.cmake module is part of the standard distribution
if ( WIN32 )
    set ( CMAKE_SYSTEM_PROGRAM_PATH ${CMAKE_SYSTEM_PROGRAM_PATH}
	    "C:/Program Files/SlikSvn/bin"
	    "C:/Program Files (x86)/SlikSvn/bin" )
    #Add more likely paths if need be
endif()

include(FindSubversion)

# extract working copy information for SOURCE_DIR into MY_XXX variables
if ( Subversion_FOUND AND OD_FROM_SVN )
    Subversion_WC_INFO( ${CMAKE_SOURCE_DIR} MY )
    set ( VCS_VERSION ${MY_WC_REVISION} )
    set ( UPDATE_CMD ${Subversion_SVN_EXECUTABLE} update )
else()
    set ( VCS_VERSION 0 )
    set ( MY_WC_REVISION 0 )
    set ( MY_WC_URL "" )

    find_package(Git)

    if( GIT_FOUND )
        # Get the latest abbreviated commit hash of the working branch
        execute_process(
          COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
          WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
          OUTPUT_VARIABLE VCS_VERSION
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
endif()

if ( EXISTS ${CMAKE_SOURCE_DIR}/external/Externals.cmake )
    execute_process(
	COMMAND ${CMAKE_COMMAND}
	    -DOpendTect_DIR=${OpendTect_DIR}
	    -DUPDATE=No
	    -P external/Externals.cmake
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} )
endif()

if ( EXISTS ${CMAKE_SOURCE_DIR}/external/Externals.cmake )
    set ( EXTERNALCMD COMMAND ${CMAKE_COMMAND}
		-DOpendTect_DIR=${OpendTect_DIR}
		-DUPDATE=Yes
		-P external/Externals.cmake )
endif()

add_custom_target( update ${UPDATE_CMD}
		  ${EXTERNALCMD}
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		  COMMENT "Updating from repositories" )
set ( INC_DIR ${CMAKE_BINARY_DIR}/include/Basic )
set ( INC_FILE ${INC_DIR}/svnversion.h )

if ( NOT EXISTS ${INC_FILE} )
    set ( OD_CREATE_INC_FILE 1 )
endif()

if ( OD_FROM_SVN OR OD_CREATE_INC_FILE )
    set ( TMPFILE ${INC_DIR}/svnversion.h.tmp )

    # write a file with the SVNVERSION define
    file( WRITE ${TMPFILE} 
	 "// Generated automatically by CMakeModules/ODSubversion.cmake${OD_LINESEP}"
	 "//${OD_LINESEP}"
	 "#ifndef mSVN_VERSION${OD_LINESEP}"
	 "#define mSVN_VERSION ${MY_WC_REVISION}${OD_LINESEP}"
	 "#define mSVN_URL \"${MY_WC_URL}\"${OD_LINESEP}"
	 "#endif${OD_LINESEP}")

    # copy the file to the final header only if the version changes
    # reduces needless rebuilds
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
			    ${TMPFILE} ${INC_DIR}/svnversion.h )
    file ( REMOVE ${TMPFILE} )
endif()
