/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipicksetman.h"
#include "uipicksetmgr.h"

#include "uibutton.h"
#include "uiioobjselgrp.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "picksettr.h"
#include "pickset.h"
#include "keystrs.h"
#include "polygon.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiPickSetMan)


uiPickSetMan::uiPickSetMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Manage PickSets/Polygons",mNoDlgTitle,
				     mODHelpKey(mPickSetManHelpID) )
				     .nrstatusflds(1).modal(false),
	           PickSetTranslatorGroup::ioContext())
{
    createDefaultUI();
    mergebut_ = selgrp_->getManipGroup()->addButton( "mergepicksets",
			  "Merge pick sets", mCB(this,uiPickSetMan,mergeSets) );
    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiPickSetMan::~uiPickSetMan()
{
}


void uiPickSetMan::ownSelChg()
{
    BufferStringSet chsnnms;
    selgrp_->getChosen( chsnnms );
    BufferString tt;
    if ( chsnnms.size() > 1 )
	tt.add( "Merge " ).add( chsnnms.getDispString(2) );
    else
	tt.add( "Merge pick sets" );

    mergebut_->setToolTip( tt );
}


void uiPickSetMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    Pick::Set ps;
    if ( !PickSetTranslator::retrieve(ps,curioobj_,true, txt) )
    {
	BufferString msg( "Read error: '" ); msg += txt; msg += "'";
	txt = msg;
    }
    else
    {
	if ( !txt.isEmpty() )
	    ErrMsg( txt );

	FixedString typ = curioobj_->pars().find( sKey::Type() );
	const bool ispoly = typ==sKey::Polygon();
	const bool havetype = typ && *typ;
	if ( havetype )
	    txt.add( "Type: " ).add( typ );

	const int sz = ps.size();
	if ( sz < 1 )
	    txt.add( havetype ? " <empty>" : "Empty Pick Set." );
	else
	{
	    txt.add( havetype ? " <" : "Size: " );
	    txt.add( sz );
	    if ( havetype )
	    {
		txt.add( ispoly ? " vertice" : " pick" );
		if ( sz > 1 )
		    txt += "s";
	    }
	    if ( !ispoly && ps[0].hasDir() )
		txt += " (with directions)";

	    if ( ispoly && sz > 2 )
	    {
		ODPolygon<double> odpoly;
		for ( int idx=0; idx<sz; idx++ )
		{
		    const Coord c( ps[idx].pos_ );
		    odpoly.add( Geom::Point2D<double>( c.x, c.y ) );
		}
		txt.add( ", area=" ).add( odpoly.area() );
	    }

	    if ( havetype )
		txt += ">";
	}

	Color col( ps.disp_.color_ ); col.setTransparency( 0 );
	txt.add( "\nColor: " ).add( col.largeUserInfoString() );
	txt.add( "\nMarker size (pixels): " ).add( ps.disp_.pixsize_ );
	txt.add( "\nMarker type: " ) .add( MarkerStyle3D::getTypeString(
				(MarkerStyle3D::Type)ps.disp_.markertype_) );
    }

    txt.add( "\n" ).add( getFileInfo() );
    setInfo( txt );
}


void uiPickSetMan::mergeSets( CallBacker* )
{
    uiPickSetMgr mgr( this, Pick::Mgr() );
    MultiID curkey; if ( curioobj_ ) curkey = curioobj_->key();
    BufferStringSet chsnnms;
    selgrp_->getChosen( chsnnms );
    mgr.mergeSets( curkey, &chsnnms );

    if ( !curkey.isEmpty() )
	selgrp_->fullUpdate( curkey );
}
