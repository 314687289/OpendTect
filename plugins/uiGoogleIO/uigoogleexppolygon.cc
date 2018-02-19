/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/


#include "uigoogleexppolygon.h"
#include "googlexmlwriter.h"
#include "uisellinest.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "oddirs.h"
#include "strmprov.h"
#include "survinfo.h"
#include "latlong.h"
#include "pickset.h"
#include "draw.h"
#include "od_helpids.h"

#include <iostream>


uiGoogleExportPolygon::uiGoogleExportPolygon( uiParent* p, const Pick::Set& ps )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport( tr("Polygon to KML")),
				 tr("Specify output parameters"),
                                 mODHelpKey(mGoogleExportPolygonHelpID) ) )
    , ps_(ps)
{
    Color defcol( ps_.dispColor() ); defcol.setTransparency( 150 );
    const OD::LineStyle ls( OD::LineStyle::Solid, 20, defcol );
    uiSelLineStyle::Setup lssu; lssu.drawstyle( false ).transparency( true );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );

    hghtfld_ = new uiGenInput( this, uiStrings::sHeight(), FloatInpSpec(100) );
    hghtfld_->attach( alignedBelow, lsfld_ );

    mImplFileNameFld(ps.name());
    fnmfld_->attach( alignedBelow, hghtfld_ );
}



bool uiGoogleExportPolygon::acceptOK()
{
    if ( ps_.isEmpty() )
	{ uiMSG().error( tr("Polygon is empty") ); return false; }
    mCreateWriter( "Polygon", SI().name() );

    TypeSet<Coord> coords;
    Pick::SetIter psiter( ps_ );
    while ( psiter.next() )
	coords += psiter.getPos();
    if ( ps_.isPolygon() )
	coords += ps_.first().pos().getXY();
    psiter.retire();

    const float reqwdth = lsfld_->getWidth() * 0.1f;
    wrr.writePolyStyle( "polygon", lsfld_->getColor(), mNINT32(reqwdth) );
    wrr.writePoly( "polygon", ps_.name(), coords, hghtfld_->getFValue() );

    return true;
}
