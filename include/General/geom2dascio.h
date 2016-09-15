#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Sep 2010
________________________________________________________________________

-*/


#include "generalmod.h"
#include "tableascio.h"

namespace PosInfo { class Line2DData; }
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

protected:

    od_istream&			strm_;
};

