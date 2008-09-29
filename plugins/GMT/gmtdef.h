#ifndef gmtdef_h
#define gmtdef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: gmtdef.h,v 1.9 2008-09-29 13:23:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "settings.h"
#include "enums.h"

namespace ODGMT
{
    enum Shape		{ Star, Circle, Diamond, Square, Triangle, Cross,
   			  Polygon, Line };
    			DeclareNameSpaceEnumUtils(Shape)
    enum Resolution	{ Full, High, Intermediate, Low, Crude };
			DeclareNameSpaceEnumUtils(Resolution)
    enum Alignment	{ Above, Below, Left, Right };
			DeclareNameSpaceEnumUtils(Alignment);

    static const char*	sShapeKeys[] = { "a", "c", "d", "s", "t", "x", "n",
					 "-", 0 };
    static const char*	sResolKeys[] = { "f", "h", "i", "l", "c" };

    static const char*	sKeyClosePS = "Close PostScript";
    static const char*	sKeyColSeq = "Color sequence";
    static const char*	sKeyCustomComm = "Custom command";
    static const char*	sKeyDataRange = "Data range";
    static const char*	sKeyDrawContour = "Draw contour";
    static const char*	sKeyDrawGridLines = "Draw gridlines";
    static const char*	sKeyDryFill = "Fill Dry";
    static const char*	sKeyDryFillColor = "Fill Color Dry";
    static const char*	sKeyFill = "Fill";
    static const char*	sKeyFillColor = "Fill Color";
    static const char*	sKeyFontSize = "Font size";
    static const char*	sKeyGMT = "GMT";
    static const char*	sKeyGMTSelKey = "808080";
    static const char*	sKeyGroupName = "Group Name";
    static const char*	sKeyLabelAlignment = "Label alignment";
    static const char*	sKeyLabelIntv = "Label Interval";
    static const char*	sKeyLegendParams = "Legend Parameters";
    static const char*	sKeyLineNames = "Line names";
    static const char*	sKeyLineStyle = "Line Style";
    static const char*	sKeyMapDim = "Map Dimension";
    static const char*	sKeyMapScale = "Map scale";
    static const char*	sKeyMapTitle = "Map Title";
    static const char*	sKeyPostLabel = "Post label";
    static const char*	sKeyPostColorBar = "Post Color bar";
    static const char*	sKeyPostStart = "Post start";
    static const char*  sKeyPostStop = "Post stop";
    static const char*	sKeyPostTitleBox = "Post title box";
    static const char*	sKeyPostTraceNrs = "Post Trace Nrs";
    static const char*	sKeyRemarks = "Remarks";
    static const char*	sKeyResolution = "Resolution";
    static const char*	sKeyShape = "Shape";
    static const char*	sKeySkipWarning = "Skip Warning";
    static const char*	sKeyUTMZone = "UTM zone";
    static const char*	sKeyWetFill = "Fill Wet";
    static const char*	sKeyWetFillColor = "Fill Color Wet";
    static const char*	sKeyWellNames = "Well names";
    static const char*	sKeyXRange = "X Range";
    static const char*	sKeyYRange = "Y Range";
};


#define mGetDefault( key, fn, var ) \
    Settings::fetch("GMT").fn(key,var); \
    

#define mSetDefault( key, fn, var ) \
    Settings::fetch("GMT").fn(key,var); \
    Settings::fetch("GMT").write();

#endif
