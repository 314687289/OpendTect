/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : September 2010
-*/

static const char* rcsID mUsedVar = " $";

#include "geom2dascio.h"

#include "posinfo2d.h"
#include "survgeom2d.h"
#include "tabledef.h"

Geom2dAscIO::Geom2dAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO( fd )
    , strm_( strm )
{
}


Table::FormatDesc* Geom2dAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Geom2D" );
    fd->bodyinfos_ += new Table::TargetInfo( "Trace Nr", IntInpSpec() );
    fd->bodyinfos_ += new Table::TargetInfo( "SP Nr", IntInpSpec() );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );

    return fd;
}


bool Geom2dAscIO::getData( PosInfo::Line2DData& geom )
{
    if ( !getHdrVals(strm_) )
	return false;

     while ( true )
     {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	const int trcnr = getIntValue(0);
	if ( mIsUdf(trcnr) )
	    continue;

	PosInfo::Line2DPos pos( getIntValue(0) );
	pos.coord_.x = getDValue( 2 );
	pos.coord_.y = getDValue( 3 );
	geom.add( pos );
     }

     return geom.positions().size();
}


bool Geom2dAscIO::getData( Survey::Geometry2D& geom )
{
    if ( !getHdrVals(strm_) )
	return false;

     while ( true )
     {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	const int trcnr = getIntValue(0);
	if ( mIsUdf(trcnr) )
	    continue;

	geom.add( getDValue(2), getDValue(3), getIntValue(0), getIntValue(1) );
     }

     return geom.size();
}
