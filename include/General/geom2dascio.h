#ifndef geom2dascio_h
#define geom2dascio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Sep 2010
 RCS:		$Id$
________________________________________________________________________

-*/


#include "generalmod.h"
#include "tableascio.h"

namespace PosInfo { class Line2DData; }
namespace Survey { class Geometry2D; }
namespace Table { class FormatDesc; }

/*!
\brief Ascii I/O for 2D Geometry.
*/

mExpClass(General) Geom2dAscIO : public Table::AscIO
{
public:
				Geom2dAscIO(const Table::FormatDesc&,
					    od_istream&);
    static Table::FormatDesc*	getDesc();
    bool			getData(PosInfo::Line2DData&);
    bool			getData(Survey::Geometry2D&);

protected:

    od_istream&			strm_;
};

#endif
