/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2009
-*/

static const char* rcsID = "$Id";

#include "uigoogleexp2dlines.h"
#include "odgooglexmlwriter.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uisellinest.h"
#include "uiseis2dfileman.h"
#include "uiseisioobjinfo.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "posinfo.h"
#include "strmprov.h"
#include "draw.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "bendpointfinder.h"
#include "latlong.h"
#include <iostream>

static const char* sIconFileName = "markerdot";


uiGoogleExport2DSeis::uiGoogleExport2DSeis( uiSeis2DFileMan* p )
    : uiDialog(p,uiDialog::Setup("Export selected 2D seismics to KML",
				 "Specify how to export","0.3.10") )
    , s2dfm_(p)
    , putallfld_(0)
    , allsel_(false)
{
    getInitialSelectedLineNames();
    const int nrsel = sellnms_.size();

    if ( !allsel_ )
	putallfld_ = new uiGenInput( this, "Export", BoolInpSpec(true,"All",
		    nrsel > 1 ? "Selected lines":"Selected line") );

    static const char* choices[]
		= { "No", "At Start/End", "At Start only", "At End only", 0 };
    putlnmfld_ = new uiGenInput( this, "Put line names",
	    			 StringListInpSpec(choices) );
    putlnmfld_->setValue( 1 );
    if ( putallfld_ )
	putlnmfld_->attach( alignedBelow, putallfld_ );

    LineStyle ls( LineStyle::Solid, 20, Color(0,0,255) );
    lsfld_ = new uiSelLineStyle( this, ls, "Line style", false, true, true );
    lsfld_->attach( alignedBelow, putlnmfld_ );

    mImplFileNameFld;
    fnmfld_->attach( alignedBelow, lsfld_ );
}


uiGoogleExport2DSeis::~uiGoogleExport2DSeis()
{
}


void uiGoogleExport2DSeis::getFinalSelectedLineNames()
{
    if ( !allsel_ )
	allsel_ = putallfld_ ? putallfld_->getBoolValue() : false;
    if ( !allsel_ )
	return;

    const uiListBox& lb( *s2dfm_->getListBox(false) );
    sellnms_.erase();
    for ( int idx=0; idx<lb.size(); idx++ )
	sellnms_.add( lb.textOfItem(idx) );
}


void uiGoogleExport2DSeis::getInitialSelectedLineNames()
{
    const uiListBox& lb( *s2dfm_->getListBox(false) );
    sellnms_.erase();
    const int nrsel = lb.nrSelected();
    allsel_ = nrsel == 0;
    for ( int idx=0; idx<lb.size(); idx++ )
    {
	if ( allsel_ || lb.isSelected(idx) )
	    sellnms_.add( lb.textOfItem(idx) );
    }
    allsel_ = sellnms_.size() == lb.size();
}


bool uiGoogleExport2DSeis::acceptOK( CallBacker* )
{
    mCreateWriter( s2dfm_->lineset_->name(), SI().name() );

    BufferString ins( "\t\t<LineStyle>\n\t\t\t<color>" );
    ins += lsfld_->getColor().getStdStr(false,-1);
    ins += "</color>\n\t\t\t<width>";
    ins += lsfld_->getWidth() * .1;
    ins += "</width>\n\t\t</LineStyle>";
    wrr.writeIconStyles( 0, 0, ins );

    getFinalSelectedLineNames();
    const uiSeisIOObjInfo& oinf( *s2dfm_->objinfo_ );
    for ( int idx=0; idx<sellnms_.size(); idx++ )
    {
	BufferStringSet attrnms;
	oinf.getAttribNamesForLine( sellnms_.get(idx), attrnms );
	if ( attrnms.isEmpty() ) continue;
	int iattr = attrnms.indexOf( LineKey::sKeyDefAttrib() );
	if ( iattr < 0 ) iattr = 0;
	addLine( wrr, sellnms_.get(idx), iattr );
    }

    wrr.close();
    return true;
}


void uiGoogleExport2DSeis::addLine( ODGoogle::XMLWriter& wrr, const char* lnm,
				    int iattr )
{
    const Seis2DLineSet& lset( *s2dfm_->lineset_ );
    LineKey lk( lnm, lset.attribute(iattr) );
    const int iln = lset.indexOf( lk );
    PosInfo::Line2DData l2dd;
    if ( iln < 0 || !lset.getGeometry(iln,l2dd) )
	return;
    const int nrposns = l2dd.posns_.size();
    if ( nrposns < 2 )
	return;

    const int lnmchoice = putlnmfld_->getIntValue();
    if ( lnmchoice != 0 && lnmchoice < 3 )
	wrr.writePlaceMark( 0, l2dd.posns_[0].coord_, lnm );
    if ( lnmchoice == 1 || lnmchoice == 3 )
	wrr.writePlaceMark( 0, l2dd.posns_[nrposns-1].coord_, lnm );

    TypeSet<Coord> crds;
    for ( int idx=0; idx<nrposns; idx++ )
	crds += l2dd.posns_[idx].coord_;

    BendPointFinder2D bpf( crds, 1 );
    bpf.execute();

    TypeSet<Coord> plotcrds;
    for ( int idx=0; idx<bpf.bendPoints().size(); idx++ )
	plotcrds += crds[ bpf.bendPoints()[idx] ];

    wrr.writeLine( 0, plotcrds, lnm );
}
