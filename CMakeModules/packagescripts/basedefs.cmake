#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define base package variables
# Author:       Nageswara
# Date:         August 2012
#RCS:           $Id$

#OpenDtect libraries
set( LIBLIST Algo AttributeEngine Attributes Basic Batch Database EarthModel General
	     Geometry MMProc MPEEngine Network NLA Seis Strat Velocity VolumeProcessing
	     PreStackProcessing EMAttrib ExpAttribs SoOD Well WellAttrib uiAttributes uiBase uiCoin
	     uiEarthModel uiEMAttrib uiExpAttribs uiFlatView uiIo uiMPE uiNLA uiODMain uiSeis uiStrat
	     uiTools uiPreStackProcessing uiVelocity uiViewer2D uiVis uiVolumeProcessing
	     uiWell uiWellAttrib uiSysAdm Usage visBase visSurvey uiCmdDriver )

set( EXECLIST od_cbvs_browse od_glxinfo od_ivfileviewer lmhostid 
	      od_main od_sysadmmain od_process_attrib od_process_attrib_em
	      od_process_prestack od_process_segyio od_process_time2depth
	      od_process_velocityconv od_process_volume od_ProgressViewer od_DispMsg
	      od_FileBrowser od_SEGYExaminer od_SeisMMBatch od_ClusterProc
	      od_process_2dgrid od_remexec od_remoteservice od_stratamp od_isopach
	      od_ReportIssue od_uiReportIssue )
set( PLUGINS HorizonAttrib GapDecon VoxelConnectivityFilter
	     uiHorizonAttrib uiPreStackViewer Annotations uiGapDecon 
	     uiGoogleIO GoogleTranslate CmdDriver uiVoxelConnectivityFilter
	     TextureAttrib uiTextureAttrib )
#Only for windows base package
set( WINEXECLIST od_start_dtect od_main_console od_runinst )

#No need to include shell scripts in windows base package.
set( TXTFILES GNU_GENERAL_PUBLIC_LICENSE.txt INSTALL.txt LICENSE.txt )
if( WIN32 )
    set( SPECFILES ${TXTFILES} )
    set( ODSCRIPTS "" )
else()
    set( SPECFILES .exec_prog .init_dtect .init_dtect_user install .lic_inst_common
		   .lic_start_common mk_datadir .start_dtect setup.od )
    set( SPECFILES ${SPECFILES} ${TXTFILES} )
    set( ODSCRIPTS od_* mksethdir mac_term macterm.in )
endif()


set( PACK "base" )
