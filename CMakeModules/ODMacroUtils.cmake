#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODMacroUtils.cmake,v 1.19 2012-02-21 16:24:58 cvskris Exp $
#_______________________________________________________________________________

# OD_INIT_MODULE - Marcro that setups a number of variables for compiling
#		   OpendTect.
#
# Input variables:
# 
# OD_MODULE_NAME			: Name of the module, or the plugin
# OD_SUBSYSTEM                          : "od" or "dgb"
# OD_MODULE_DEPS			: List of other modules that this
#					  module is dependent on.
# OD_USEPROG				: Whether to include include/Prog 
# OD_USECOIN				: Dependency on Coin is enabled if set.
# OD_USEQT				: Dependency on Qt is enabled if set.
#					  value should be either Core, Sql, Gui
#					  or OpenGL
# OD_USEZLIB				: Dependency on zlib is enabled if set.
# OD_IS_PLUGIN				: Tells if this is a plugin (if set)
# OD_PLUGINMODULES			: A list of eventual sub-modules of
#					  a plugin. Each submodule must have an
#					  plugins/{OD_MODULE_NAME}/src/${PLUGINMODULE}/CMakeFile.txt
#####################################
#
# Output variables:
#
# OD_${OD_MODULE_NAME}_INCLUDEPATH 	: The path(s) to the headerfiles of the
#					  module. Normally one single one, but
#					  may have multiple paths in plugins.
# OD_${OD_MODULE_NAME}_DEPS		: The modules this module is dependent
#					  on.
# OD_${OD_MODULE_NAME}_RUNTIMEPATH	: The runtime path for its own library, 
#					  and all external libraries it is
#					  dependent on.
# OD_MODULE_NAMES_${OD_SUBSYSTEM}	: A list of all modules
#####################################
#
# Internal variables
#
# OD_MODULE_INCLUDEPATH		: The includepath needed to compile the source-
#				  files of the module.
# OD_MODULE_RUNTIMEPATH		: All directories that are needed at runtime
# OD_MODULE_INTERNAL_LIBS	: All OD libraries needed for the module
# OD_MODULE_EXTERNAL_LIBS	: All external libraries needed for the module

MACRO( OD_INIT_MODULE )

#Add this module to the list
SET( OD_MODULE_NAMES_${OD_SUBSYSTEM} ${OD_MODULE_NAMES_${OD_SUBSYSTEM}} ${OD_MODULE_NAME} PARENT_SCOPE )

#Add all module dependencies
IF(OD_MODULE_DEPS)
    FOREACH( DEP ${OD_MODULE_DEPS} )
	OD_ADD_DEPS( ${DEP} )
    ENDFOREACH()
ENDIF()


IF(OD_USECOIN)
    OD_SETUP_COIN()
ENDIF()

IF( UNIX AND OD_USEZLIB )
    LIST(APPEND OD_MODULE_INCLUDEPATH ${ZLIB_INCLUDE_DIR} )
    LIST(APPEND OD_MODULE_EXTERNAL_LIBS ${ZLIB_LIBRARY} )
ENDIF()

IF(OD_USEOSG)
    OD_SETUP_OSG()
ENDIF()

#Add Qt-stuff
IF(OD_USEQT)
   OD_SETUP_QT()
ENDIF(OD_USEQT)

#Add current module to include-path
IF (OD_IS_PLUGIN)
    SET( PLUGINDIR ${CMAKE_SOURCE_DIR}/plugins/${OD_MODULE_NAME})
    LIST( APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH ${PLUGINDIR})
    FOREACH( OD_PLUGINSUBDIR ${OD_PLUGINMODULES} )
	LIST(APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH
	    ${PLUGINDIR}/include/${OD_PLUGINSUBDIR})
	INCLUDE(${PLUGINDIR}/src/${OD_PLUGINSUBDIR}/CMakeLists.txt)
    ENDFOREACH()

    # Write alo-file
    FOREACH( PLUGINEXEC ${OD_PLUGIN_EXECS} )
	SET ( OD_ALOFILE ${CMAKE_BINARY_DIR}/plugins/${OD_PLFSUBDIR}/${PLUGINEXEC}.${OD_SUBSYSTEM}.alo )
	IF ( EXISTS ${OD_ALOFILE} )
	    FILE( APPEND ${OD_ALOFILE} ${OD_MODULE_NAME} "\n" )
	ELSE()
	    FILE( WRITE ${OD_ALOFILE} ${OD_MODULE_NAME} "\n" )
	ENDIF()
    ENDFOREACH()
ELSE()
    LIST(APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH
	${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
    LIST(APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH
        ${CMAKE_SOURCE_DIR}/src/${OD_MODULE_NAME}/${CMAKE_BUILD_TYPE} )
ENDIF(OD_IS_PLUGIN)

guess_runtime_library_dirs( EXTERNAL_RUNTIMEPATH ${OD_MODULE_EXTERNAL_LIBS} )
LIST(APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${EXTERNAL_RUNTIMEPATH} )
LIST(APPEND OD_MODULE_INCLUDEPATH ${OD_${OD_MODULE_NAME}_INCLUDEPATH} )
LIST(APPEND OD_MODULE_RUNTIMEPATH ${OD_${OD_MODULE_NAME}_RUNTIMEPATH} )

#Clean up the lists
LIST(REMOVE_DUPLICATES OD_MODULE_INCLUDEPATH)
IF(  OD_MODULE_RUNTIMEPATH )
    LIST(REMOVE_DUPLICATES OD_MODULE_RUNTIMEPATH)
ENDIF()

#Export dependencies
SET( OD_${OD_MODULE_NAME}_DEPS ${OD_MODULE_DEPS} PARENT_SCOPE )
SET( OD_${OD_MODULE_NAME}_INCLUDEPATH
     ${OD_${OD_MODULE_NAME}_INCLUDEPATH} PARENT_SCOPE)
SET( OD_${OD_MODULE_NAME}_RUNTIMEPATH
     ${OD_${OD_MODULE_NAME}_RUNTIMEPATH} PARENT_SCOPE)

#Extract static libraries
FOREACH( STATIC_LIB ${OD_MODULE_STATIC_LIBS} )
    GET_FILENAME_COMPONENT( STATIC_LIB_NAME ${STATIC_LIB} NAME )
    SET( STATIC_LIB_DIR
         ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${OD_MODULE_NAME}.dir/${STATIC_LIB_NAME}.dir )
    SET ( SHARED_LIB_COMMAND ${CMAKE_AR} x ${STATIC_LIB} )
    IF ( NOT EXISTS ${STATIC_LIB_DIR} )
	FILE( MAKE_DIRECTORY ${STATIC_LIB_DIR} )

	EXECUTE_PROCESS( 
	    COMMAND ${SHARED_LIB_COMMAND}
	    WORKING_DIRECTORY ${STATIC_LIB_DIR} )
    ENDIF()

    FILE( GLOB STATIC_LIB_FILES ${STATIC_LIB_DIR}/*.o )
    LIST( APPEND OD_STATIC_OUTFILES ${STATIC_LIB_FILES} )

ENDFOREACH()

#Setup library & its deps
IF( OD_LIB_LINKER_NEEDS_ALL_LIBS )
    SET( OD_LIB_DEP_LIBS ${EXTRA_LIBS} ${OD_MODULE_DEPS} )
ELSE()
    SET( OD_EXEC_DEP_LIBS ${EXTRA_LIBS} ${OD_MODULE_INTERNAL_LIBS} )
ENDIF()

ADD_LIBRARY( ${OD_MODULE_NAME} SHARED ${OD_MODULE_SOURCES} ${QT_MOC_OUTFILES}
	     ${OD_STATIC_OUTFILES} )
TARGET_LINK_LIBRARIES(
        ${OD_MODULE_NAME}
	${OD_LIB_DEP_LIBS}
	${OD_MODULE_EXTERNAL_LIBS}
	${OD_MODULE_LINK_OPTIONS}
     )

INSTALL(TARGETS
        ${OD_MODULE_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib )


#Setup common things for batch-programs
IF( OD_MODULE_EXECS OR OD_MODULE_BATCHPROGS )
    SET ( OD_RUNTIMELIBS ${OD_MODULE_NAME} ${OD_MODULE_DEPS})
    SET ( OD_USEPROG 1 )
ENDIF()

#Add executable targets
IF(OD_MODULE_EXECS)
    FOREACH( EXEC ${OD_MODULE_EXECS} )
	GET_FILENAME_COMPONENT( TARGET_NAME ${EXEC} NAME_WE )
	ADD_EXECUTABLE( ${TARGET_NAME} ${EXEC}.cc )
	TARGET_LINK_LIBRARIES(
	    ${TARGET_NAME}
	    ${OD_EXEC_DEP_LIBS}
	    ${OD_RUNTIMELIBS} )
        IF( OD_CREATE_LAUNCHERS )
	    create_target_launcher( ${TARGET_NAME}
		RUNTIME_LIBRARY_DIRS
		${OD_MODULE_RUNTIMEPATH} )
        ENDIF( OD_CREATE_LAUNCHERS )
	INSTALL(TARGETS
		${TARGET_NAME}
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib )
    ENDFOREACH()

ENDIF(OD_MODULE_EXECS)

IF(OD_MODULE_BATCHPROGS)
    #Add dep on Batch if there are batch-progs
    IF ( OD_MODULE_BATCHPROGS )
	LIST( APPEND OD_RUNTIMELIBS "Batch" "Network" )
	LIST( REMOVE_DUPLICATES OD_RUNTIMELIBS )
    ENDIF()


    FOREACH( EXEC ${OD_MODULE_BATCHPROGS} )
	GET_FILENAME_COMPONENT( TARGET_NAME ${EXEC} NAME_WE )
	ADD_EXECUTABLE( ${TARGET_NAME} ${EXEC}.cc )
	TARGET_LINK_LIBRARIES(
	    ${TARGET_NAME}
	    ${OD_EXEC_DEP_LIBS}
	    ${OD_RUNTIMELIBS} )
	SET_PROPERTY( TARGET ${TARGET_NAME} PROPERTY COMPILE_DEFINITIONS
		      __prog__ )
	IF( OD_CREATE_LAUNCHERS )
	    create_target_launcher( ${TARGET_NAME}
		RUNTIME_LIBRARY_DIRS
		${OD_MODULE_RUNTIMEPATH} )
	INSTALL(TARGETS
		${TARGET_NAME}
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib )
	ENDIF( OD_CREATE_LAUNCHERS )
    ENDFOREACH()

ENDIF(OD_MODULE_BATCHPROGS)

IF(OD_USEPROG)
    LIST(APPEND OD_MODULE_INCLUDEPATH
		${OpendTect_DIR}/include/Prog)
ENDIF( OD_USEPROG )


#Set current include_path
INCLUDE_DIRECTORIES( ${OD_MODULE_INCLUDEPATH} )

ENDMACRO(OD_INIT_MODULE)

MACRO( OD_ADD_DEPS DEP )
#Check if dep is already in the list
LIST(FIND OD_${OD_MODULE_NAME}_DEPS ${DEP} INDEX)
IF(${INDEX} EQUAL -1)
    #Add dependency
    LIST(APPEND OD_MODULE_INTERNAL_LIBS ${DEP} )
    
    #Add dependencies of dependencies
    FOREACH( DEPLIB ${OD_${DEP}_DEPS} )
	OD_ADD_DEPS( ${DEPLIB} )
    ENDFOREACH()

    #Add dependencies to include-path
    LIST(APPEND OD_MODULE_INCLUDEPATH ${OD_${DEP}_INCLUDEPATH} )
    LIST(APPEND OD_MODULE_RUNTIMEPATH ${OD_${DEP}_RUNTIMEPATH} )
ENDIF()

ENDMACRO(OD_ADD_DEPS)


# OD_ADD_PLUGIN_SOURCES(SOURCES) - Adds sources in a submodule of a plugin
#
# Input variables:
# 
# OD_PLUGINSUBDIR			: Name sub-module of the plugin
# SOURCES				: List of sources to add
#
# Output:
# OD_MODULE_SOURCES

MACRO ( OD_ADD_PLUGIN_SOURCES )
    FOREACH( SRC ${ARGV} )
	LIST( APPEND OD_MODULE_SOURCES src/${OD_PLUGINSUBDIR}/${SRC} )
    ENDFOREACH()
ENDMACRO()

# OD_ADD_PLUGIN_EXECS(SOURCES) - Adds sources in a submodule of a plugin
#
# Input variables:
# 
# OD_PLUGINSUBDIR			: Name sub-module of the plugin
# SOURCES				: List of sources to add
#
# Output:
# OD_MODULE_EXECS

MACRO ( OD_ADD_PLUGIN_EXECS )
    FOREACH( SRC ${ARGV} )
	LIST( APPEND OD_MODULE_EXECS src/${OD_PLUGINSUBDIR}/${SRC} )
    ENDFOREACH()
ENDMACRO()


# OD_ADD_PLUGIN_BATCHPROGS(SOURCES) - Adds sources in a submodule of a plugin
#
# Input variables:
# 
# OD_PLUGINSUBDIR			: Name sub-module of the plugin
# SOURCES				: List of sources to add
#
# Output:
# OD_MODULE_BATCHPROGS

MACRO ( OD_ADD_PLUGIN_BATCHPROGS )
    FOREACH( SRC ${ARGV} )
	LIST( APPEND OD_MODULE_BATCHPROGS src/${OD_PLUGINSUBDIR}/${SRC} )
    ENDFOREACH()
ENDMACRO()
