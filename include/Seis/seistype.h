#ifndef seistype_h
#define seistype_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		29-5-99
 RCS:		$Id: seistype.h,v 1.16 2009-01-27 11:44:12 cvsranojay Exp $
________________________________________________________________________

*/

#include "gendefs.h"
class IOPar;

namespace Seis
{

    mGlobal enum SelType	{ Range, Table, Polygon };
    mGlobal const char*		nameOf(SelType);
    mGlobal SelType		selTypeOf(const char*);
    mGlobal const char**	selTypeNames();

    enum ReadMode	{ PreScan, Scan, Prod };

    enum GeomType	{ Vol, VolPS, Line, LinePS };
    mGlobal inline bool		is2D( GeomType gt )
    			{ return gt > VolPS; }
    mGlobal inline bool		is3D( GeomType gt )
    			{ return gt < Line; }
    mGlobal inline bool		isPS( GeomType gt )
    			{ return gt == VolPS || gt == LinePS; }
    mGlobal inline int		dimSize( GeomType gt )
			{ return gt == Line ? 2 : (gt == VolPS ? 4 : 3); }
    mGlobal const char*		nameOf(GeomType);
    mGlobal GeomType		geomTypeOf(const char*);
    mGlobal inline GeomType	geomTypeOf( bool is2d, bool isps )
			{ return is2d ? (isps?LinePS:Line) : (isps?VolPS:Vol); }
    mGlobal const char**	geomTypeNames();
    mGlobal void		putInPar(GeomType,IOPar&);
    mGlobal bool		getFromPar(const IOPar&,GeomType&);


    enum DataType       { Ampl, Dip, Frequency, Phase, AVOGradient, 
			  Azimuth, Classification, UnknowData };
    mGlobal bool		isAngle(DataType);
    mGlobal const char*		nameOf(DataType);
    mGlobal DataType		dataTypeOf(const char*);
    mGlobal const char**	dataTypeNames();

    enum WaveType       { P, Sh, Sv, UnknowWave };
    mGlobal const char*		nameOf(WaveType);
    mGlobal WaveType		waveTypeOf(const char*);
    mGlobal const char**	waveTypeNames();
};


#endif
