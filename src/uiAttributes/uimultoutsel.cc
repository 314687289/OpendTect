/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimultoutsel.cc,v 1.1 2009-01-07 11:19:50 cvshelene Exp $";

#include "uimultoutsel.h"

#include "attribdesc.h"
#include "attribprovider.h"
#include "uilistbox.h"

using namespace Attrib;

uiMultOutSel::uiMultOutSel( uiParent* p, const Desc& desc )
	: uiDialog(p,Setup("Multiple components selection",
		    	   "Select the outputs to compute", mTODOHelpID))
{
    BufferStringSet outnames;
    Desc* tmpdesc = new Desc( desc );
    tmpdesc->ref();
    fillInAvailOutNames( tmpdesc, outnames );
    const bool dodlg = outnames.size() > 1;
    if ( dodlg )
	createMultOutDlg( outnames );

    tmpdesc->unRef();
}


void uiMultOutSel::fillInAvailOutNames( Desc* desc,
					BufferStringSet& outnames ) const
{
    BufferString errmsg;
    Provider* tmpprov = Provider::create( *desc, errmsg );
    if ( !tmpprov ) return;
    tmpprov->ref();

    //compute and set refstep, needed to get nr outputs for some attribs
    //( SpecDecomp for ex )
    tmpprov->computeRefStep();

    tmpprov->getCompNames( outnames );
    tmpprov->unRef();
}


void uiMultOutSel::createMultOutDlg( const BufferStringSet& outnames )
{
    outlistfld_ = new uiListBox( this );
    outlistfld_->setMultiSelect();
    outlistfld_->addItems( outnames );
}


void uiMultOutSel::getSelectedOutputs( TypeSet<int>& selouts ) const
{
    if ( outlistfld_ )
	outlistfld_->getSelectedItems( selouts );
}


void uiMultOutSel::getSelectedOutNames( BufferStringSet& seloutnms ) const
{
    if ( outlistfld_ )
	outlistfld_->getSelectedItems( seloutnms );
}


bool uiMultOutSel::doDisp() const
{
    return outlistfld_;
}
