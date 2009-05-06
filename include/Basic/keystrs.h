#ifndef keystrs_h
#define keystrs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2002

 RCS:		$Id: keystrs.h,v 1.51 2009-05-06 09:31:42 cvsranojay Exp $
________________________________________________________________________

-*/
 
 
#include "gendefs.h"
#include "fixedstring.h"

#undef mImpl


#ifdef KEYSTRS_IMPL
# define mImpl(s) = s
#ifdef __msvc__
#define mExt mBasicExtern
#else 
# define mExt
#endif
#else
# define mImpl(s) /* empty */
# define mExt mBasicExtern
#endif



/*!\brief is used for defining key strings that are 'global'.

Some standard key strings are shared between otherwise unrelated modules.
To make sure no artificial dependencies are created, such a key can be added
to this namespace.

*/

namespace sKey
{

    mExt FixedString	Attribute 	mImpl("Attribute");
    mExt FixedString	Attributes	mImpl("Attributes");
    mExt FixedString	Azimuth		mImpl("Azimuth");
    mExt FixedString	Color		mImpl("Color");
    mExt FixedString	Cube		mImpl("Cube");
    mExt FixedString	DataType	mImpl("DataType");
    mExt FixedString	Depth		mImpl("Depth");
    mExt FixedString	Desc		mImpl("Description");
    mExt FixedString	Factor		mImpl("Factor");
    mExt FixedString	FileName	mImpl("File name");
    mExt FixedString	Filter		mImpl("Filter");
    mExt FixedString	FloatUdf	mImpl("1e30");
    mExt FixedString	Geometry	mImpl("Geometry");
    mExt FixedString	ID		mImpl("ID");
    mExt FixedString	IOSelection	mImpl("I/O Selection");
    mExt FixedString	Keys		mImpl("Keys");
    mExt FixedString	LineKey		mImpl("Line key");
    mExt FixedString	Log		mImpl("Log");
    mExt FixedString	Name		mImpl("Name");
    mExt FixedString	No		mImpl("No");
    mExt FixedString	None		mImpl("None");
    mExt FixedString	Offset		mImpl("Offset");
    mExt FixedString	Output		mImpl("Output");
    mExt FixedString	Pars		mImpl("Parameters");
    mExt FixedString	Polygon		mImpl("Polygon");
    mExt FixedString	Position	mImpl("Position");
    mExt FixedString	Random		mImpl("Random");
    mExt FixedString	Range		mImpl("Range");
    mExt FixedString	Scale		mImpl("Scale");
    mExt FixedString	Selection	mImpl("Selection");
    mExt FixedString	Subsample	mImpl("Subsample");
    mExt FixedString	Shortcuts	mImpl("Shortcuts");
    mExt FixedString	Size		mImpl("Size");
    mExt FixedString	Steering	mImpl("Steering");
    mExt FixedString	StratRef	mImpl("Strat Level");
    mExt FixedString	Subsel		mImpl("Subsel");
    mExt FixedString	Surface		mImpl("Surface");
    mExt FixedString	Survey		mImpl("Survey");
    mExt FixedString	Table		mImpl("Table");
    mExt FixedString	Target		mImpl("Target");
    mExt FixedString	Time		mImpl("Time");
    mExt FixedString	Title		mImpl("Title");
    mExt FixedString    TraceNr		mImpl("Trace number");
    mExt FixedString	Type		mImpl("Type");
    mExt FixedString	Undef		mImpl("Undefined");
    mExt FixedString	Value		mImpl("Value");
    mExt FixedString	Yes		mImpl("Yes");

    mExt FixedString	Average		mImpl("Average");
    mExt FixedString	Maximum		mImpl("Maximum");
    mExt FixedString	Median		mImpl("Median");
    mExt FixedString	Minimum		mImpl("Minimum");
    mExt FixedString	StdDev		mImpl("StdDev");
    mExt FixedString	Sum		mImpl("Sum");

    mExt FixedString	BinIDSel	mImpl("BinID selection");
    mExt FixedString	InlRange	mImpl("In-line range");
    mExt FixedString	FirstInl	mImpl("First In-line");
    mExt FixedString	LastInl		mImpl("Last In-line");
    mExt FixedString	StepInl		mImpl("Step In-line");
    mExt FixedString	StepOutInl	mImpl("Stepout In-line");
    mExt FixedString	CrlRange	mImpl("Cross-line range");
    mExt FixedString	FirstCrl	mImpl("First Cross-line");
    mExt FixedString	LastCrl		mImpl("Last Cross-line");
    mExt FixedString	StepCrl		mImpl("Step Cross-line");
    mExt FixedString	StepOutCrl	mImpl("Stepout Cross-line");
    mExt FixedString	ZRange		mImpl("Z range");
    mExt FixedString    FirstTrc	mImpl("First Trace");
    mExt FixedString	LastTrc		mImpl("Last Trace");
    mExt FixedString    TrcRange        mImpl("Trace Range");

    mExt FixedString	TmpStor		mImpl("Temporary storage location");
    mExt FixedString	LogFile		mImpl("Log file");
    mExt FixedString	Version		mImpl("Version");

	
    mExt FixedString	Default		mImpl("Default");
    mExt FixedString	DefCube		mImpl("Default.Cube");
    mExt FixedString	DefLineSet	mImpl("Default.LineSet");
    mExt FixedString	DefLine		mImpl("Default.Line");
    mExt FixedString	DefAttribute	mImpl("Default.Attribute");
    mExt FixedString	DefPS3D		mImpl("Default.PS3D Data Store");
    mExt FixedString	DefPS2D		mImpl("Default.PS2D Data Store");
    mExt FixedString	DefWavelet	mImpl("Default.Wavelet");

};

#undef mExt
#undef mImpl

#endif
