#ifndef uiodstdmenu_h
#define uiodstdmenu_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: uiodstdmenu.h,v 1.71 2011-06-09 10:54:53 cvsumesh Exp $
________________________________________________________________________

-*/


/*
   These are the menu IDs of standard menu items in OpendTect.
   Note that all 'nodes' (i.e. items that have sub-items) are also available
   through the interface of the uiODMenuMgr directly, e.g.:
   uiPopupMenu* viewmnu = ODMainWin()->menuMgr().viewMnu();

 */


#define mFileMnu		1000
#define mProcMnu		2000
#define mWinMnu			3000
#define mViewMnu		4000
#define mUtilMnu		5000
#define mAppMnu			6000
#define mHelpMnu		1000000000

/* 'File' menu */

#define mFileSessMnu		(mFileMnu + 100)
#define mFileImpMnu		(mFileMnu + 200)
#define mFileExpMnu		(mFileMnu + 400)
#define mFileManMnu		(mFileMnu + 600)
#define mFilePreLoadMnu		(mFileMnu + 800)

#define mManSurveyMnuItm	(mFileMnu + 10)
#define mExitMnuItm		(mFileMnu + 20)
#define mSessSaveMnuItm		(mFileSessMnu + 10)
#define mSessRestMnuItm		(mFileSessMnu + 20)
#define mSessAutoMnuItm		(mFileSessMnu + 30)

#define mImpSeisCBVSMnuItm	(mFileImpMnu + 10)
#define mImpSeisSEGYMnuItm	(mFileImpMnu + 11)
#define mImpSeisSEGYDirectMnuItm (mFileImpMnu + 12)
#define mImpSeisSimple3DMnuItm	(mFileImpMnu + 15)
#define mImpSeisSimple2DMnuItm	(mFileImpMnu + 16)
#define mImpSeisSimplePS3DMnuItm (mFileImpMnu + 17)
#define mImpSeisSimplePS2DMnuItm (mFileImpMnu + 18)
#define mImpSeisCBVSOtherSurvMnuItm (mFileImpMnu + 19)

#define mImpHorAsciiMnuItm	(mFileImpMnu + 30)
#define mImpHorAsciiAttribMnuItm (mFileImpMnu + 31)
#define mImpHor2DAsciiMnuItm	(mFileImpMnu + 32)
#define mImpWellAsciiTrackMnuItm (mFileImpMnu + 40)
#define mImpWellAsciiLogsMnuItm	(mFileImpMnu + 41)
#define mImpWellAsciiMarkersMnuItm (mFileImpMnu + 42)
#define mImpWellSEGYVSPMnuItm	(mFileImpMnu + 45)
#define mImpWellSimpleMnuItm	(mFileImpMnu + 46)
#define mImpFaultMnuItm		(mFileImpMnu + 50)
#define mImpFaultSSMnuItm	(mFileImpMnu + 51)
#define mImpFaultSSAscii3DMnuItm (mFileImpMnu + 52)
#define mImpFaultSSAscii2DMnuItm (mFileImpMnu + 53)
#define mImpPickMnuItm		(mFileImpMnu + 60)
#define mImpPickAsciiMnuItm	(mFileImpMnu + 61)
#define mImpWvltMnuItm		(mFileImpMnu + 70)
#define mImpWvltAsciiMnuItm	(mFileImpMnu + 71)
#define mImpMuteDefMnuItm	(mFileImpMnu + 80)
#define mImpMuteDefAsciiMnuItm	(mFileImpMnu + 81)
#define mImpVelocityMnuItm	(mFileImpMnu + 90)
#define mImpVelocityAsciiMnuItm	(mFileImpMnu + 91)
#define mImpPDFMnuItm		(mFileImpMnu + 100)
#define mImpPDFAsciiMnuItm	(mFileImpMnu + 101)
#define mImpPVDSMnuItm		(mFileImpMnu + 110)
#define mImpPVDSAsciiMnuItm	(mFileImpMnu + 111)

#define mExpSeisSEGY3DMnuItm	(mFileExpMnu + 11)
#define mExpSeisSEGY2DMnuItm	(mFileExpMnu + 12)
#define mExpSeisSEGYPS3DMnuItm	(mFileExpMnu + 13)
#define mExpSeisSEGYPS2DMnuItm	(mFileExpMnu + 14)
#define mExpSeisSimple3DMnuItm	(mFileExpMnu + 15)
#define mExpSeisSimple2DMnuItm	(mFileExpMnu + 16)
#define mExpSeisSimplePS3DMnuItm (mFileExpMnu + 17)
#define mExpSeisSimplePS2DMnuItm (mFileExpMnu + 18)

#define mExpHorAscii3DMnuItm	(mFileExpMnu + 30)
#define mExpHorAscii2DMnuItm	(mFileExpMnu + 31)
#define mExpFltAsciiMnuItm	(mFileExpMnu + 50)
#define mExpFltSSAsciiMnuItm	(mFileExpMnu + 51)
#define mExpPickMnuItm		(mFileExpMnu + 60)
#define mExpPickAsciiMnuItm	(mFileExpMnu + 61)
#define mExpWvltMnuItm		(mFileExpMnu + 70)
#define mExpWvltAsciiMnuItm	(mFileExpMnu + 71)
#define mExpMuteDefMnuItm	(mFileExpMnu + 80)
#define mExpMuteDefAsciiMnuItm	(mFileExpMnu + 81)
#define mExpPDFMnuItm		(mFileExpMnu + 90)
#define mExpPDFAsciiMnuItm	(mFileExpMnu + 91)

#define mManSeis3DMnuItm	(mFileManMnu + 10)
#define mManSeis2DMnuItm	(mFileManMnu + 11)
#define mManHor3DMnuItm		(mFileManMnu + 20)
#define mManHor2DMnuItm		(mFileManMnu + 21)
#define mManFaultMnuItm		(mFileManMnu + 30)
#define mManFaultStickMnuItm	(mFileManMnu + 31)
#define mManWellMnuItm		(mFileManMnu + 40)
#define mManPickMnuItm		(mFileManMnu + 50)
#define mManWvltMnuItm		(mFileManMnu + 60)
#define mManAttrMnuItm		(mFileManMnu + 70)
#define mManNLAMnuItm		(mFileManMnu + 80)
#define mManSessMnuItm		(mFileManMnu + 90)
#define mManStratMnuItm		(mFileManMnu + 95)
#define mManPDFMnuItm		(mFileManMnu + 100)
#define mManGeomItm		(mFileManMnu + 110)
#define mPreLoadSeisMnuItm	(mFilePreLoadMnu + 10)
#define mPreLoadHorMnuItm	(mFilePreLoadMnu + 11)


/* 'Processing' menu */

#define mUseHorMnu       	(mProcMnu + 30)

#define mEditAttrMnuItm		(mProcMnu + 10)
#define mEdit2DAttrMnuItm	(mProcMnu + 11)
#define mEdit3DAttrMnuItm	(mProcMnu + 12)
#define mSeisOutMnuItm		(mProcMnu + 20)
#define mSeisOut2DMnuItm	(mProcMnu + 21)
#define mSeisOut3DMnuItm	(mProcMnu + 22)
#define mCreateSurf2DMnuItm	(mUseHorMnu + 1)
#define mCompBetweenHor2DMnuItm	(mUseHorMnu + 2)
#define mCompAlongHor2DMnuItm	(mUseHorMnu + 3)
#define mCreateSurf3DMnuItm	(mUseHorMnu + 4)
#define mCompBetweenHor3DMnuItm	(mUseHorMnu + 5)
#define mCompAlongHor3DMnuItm	(mUseHorMnu + 6)
#define mXplotMnuItm		(mProcMnu + 40)
#define mAXplotMnuItm		(mAppMnu + 60)
#define mOpenXplotMnuItm	(mAppMnu + 70)
#define mReStartMnuItm		(mProcMnu + 50)


/* 'Windows' menu */

#define mAddSceneMnuItm		(mWinMnu + 10)
#define mAddTmeDepthMnuItm	(mWinMnu + 11)
#define mCascadeMnuItm		(mWinMnu + 20)
#define mTileAutoMnuItm		(mWinMnu + 30)
#define mTileHorMnuItm		(mWinMnu + 31)
#define mTileVerMnuItm		(mWinMnu + 32)
#define mSceneSelMnuItm		(mWinMnu + 40)


/* "View' menu */

#define mWorkAreaMnuItm		(mViewMnu + 10)
#define mBaseMapMnuItm		(mViewMnu + 11)
#define mZScaleMnuItm		(mViewMnu + 20)

#define mViewStereoMnu		(mViewMnu + 100)

#define mStereoOffMnuItm	(mViewStereoMnu + 10)
#define mStereoRCMnuItm		(mViewStereoMnu + 20)
#define mStereoQuadMnuItm	(mViewStereoMnu + 30)
#define mStereoOffsetMnuItm	(mViewStereoMnu + 40)

#define mViewIconsMnuItm	(mViewMnu + 200)


/* 'Utilities' menu */

#define mUtilSettingMnu		(mUtilMnu + 100)

#define mBatchProgMnuItm	(mUtilMnu + 10)
#define mPluginsMnuItm		(mUtilMnu + 20)
#define mPosconvMnuItm		(mUtilMnu + 25)
#define mCrDevEnvMnuItm		(mUtilMnu + 30)
#define mShwLogFileMnuItm	(mUtilMnu + 40)
#define mUpdateMnuItem		(mUtilMnu + 50)
#define mDumpDataPacksMnuItm	(mUtilMnu + 99)
#define mSettFontsMnuItm	(mUtilSettingMnu + 10)
#define mSettMouseMnuItm	(mUtilSettingMnu + 20)
#define mSettLkNFlMnuItm	(mUtilSettingMnu + 30)
#define mSettGeneral		(mUtilSettingMnu + 40)
#define mSettSurvey		(mUtilSettingMnu + 41)
#define mSettShortcutsMnuItm	(mUtilSettingMnu + 50)


/* 'Help' menu */

#define mHelpMnuBase		(mHelpMnu + 100)
#define mAdminMnuItm		(mHelpMnuBase + 1)
#define mProgrammerMnuItm	(mHelpMnuBase + 2)
#define mHelpAboutMnuBase	(mHelpMnuBase + 100)
#define mHelpCreditsMnuBase	(mHelpMnuBase + 200)
#define mHelpVarMnuBase		(mHelpMnuBase + 300)



#endif
