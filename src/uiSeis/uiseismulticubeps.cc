/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uiseismulticubeps.cc,v 1.3 2008-09-09 08:41:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseismulticubeps.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uiioobjsel.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "seismulticubeps.h"
#include "seistrctr.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "iodirentry.h"
#include "ioobj.h"

class uiSeisMultiCubePSEntry
{
public:
    	uiSeisMultiCubePSEntry( IOObj* i )
	    : ioobj_(i), offs_(mUdf(float))	{}
	~uiSeisMultiCubePSEntry()		{ delete ioobj_; }

	IOObj*	ioobj_;
	float	offs_;
};


uiSeisMultiCubePS::uiSeisMultiCubePS( uiParent* p )
	: uiDialog(p, uiDialog::Setup("MultiCube Pre-Stack data store",
		   "Create MultiCube Pre-Stack data store",mTODOHelpID))
	, ctio_(*mMkCtxtIOObj(SeisPS3D))
	, cubefld_(0)
	, curselidx_(-1)
{
    ctio_.ctxt.forread = false;
    ctio_.ctxt.deftransl = ctio_.ctxt.trglobexpr = "MultiCube";

    fillEntries();
    if ( entries_.isEmpty() )
    {
	new uiLabel( this, "No cubes found.\n\nPlease import data first." );
	return;
    }

    uiLabeledListBox* cubesllb = new uiLabeledListBox( this, "Available cubes",
					false, uiLabeledListBox::AboveMid );
    cubefld_ = cubesllb->box();
    fillBox( cubefld_ );
    cubefld_->setPrefWidthInChar( 30 );

    uiButtonGroup* bgrp = new uiButtonGroup( this, "", true );
    uiToolButton* rbut = new uiToolButton( bgrp, "Right button",
	    			mCB(this,uiSeisMultiCubePS,addCube) );
    rbut->setArrowType( uiToolButton::RightArrow );
    uiToolButton* lbut = new uiToolButton( bgrp, "Left button",
	    			mCB(this,uiSeisMultiCubePS,rmCube) );
    lbut->setArrowType( uiToolButton::LeftArrow );
    bgrp->attach( centeredRightOf, cubesllb );

    uiLabeledListBox* selllb = new uiLabeledListBox( this, "Used cubes",
					false, uiLabeledListBox::AboveMid );
    selfld_ = selllb->box();
    selllb->attach( rightTo, cubesllb );
    selllb->attach( ensureRightOf, bgrp );
    selfld_->selectionChanged.notify( mCB(this,uiSeisMultiCubePS,selChg) );
    selfld_->setPrefWidthInChar( 30 );

    offsfld_ = new uiGenInput( this, "Offset", 
			       FloatInpSpec().setName("Offset") );
    offsfld_->attach( alignedBelow, selllb );

    uiSeparator* sep = new uiSeparator( this, "Hor sep", true, false );
    sep->attach( stretchedBelow, offsfld_ );
    sep->attach( ensureBelow, cubesllb );

    outfld_ = new uiIOObjSel( this, ctio_, "Output data store" );
    outfld_->attach( alignedBelow, bgrp );
    outfld_->attach( ensureBelow, sep );
}


uiSeisMultiCubePS::~uiSeisMultiCubePS()
{
    delete ctio_.ioobj;
    deepErase( entries_ );
    deepErase( selentries_ );
    delete &ctio_;
}


const IOObj* uiSeisMultiCubePS::createdIOObj() const
{
    return ctio_.ioobj;
}


void uiSeisMultiCubePS::fillEntries()
{
    IOM().to( ctio_.ctxt.getSelKey() );
    IODirEntryList del( IOM().dirPtr(), SeisTrcTranslatorGroup::ioContext() );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IODirEntry& de = *del[idx];
	if ( !de.ioobj || !de.ioobj->isReadDefault() ) continue;

	entries_ += new uiSeisMultiCubePSEntry( de.ioobj->clone() );
    }
}


void uiSeisMultiCubePS::recordEntryOffs()
{
    if ( curselidx_ < 0 || selentries_.isEmpty() ) return;
    if ( curselidx_ >= selentries_.size() )
	curselidx_ = selentries_.size() - 1;

    selentries_[curselidx_]->offs_ = offsfld_->getfValue();
}


void uiSeisMultiCubePS::selChg( CallBacker* cb )
{
    const int selidx = selfld_->currentItem();
    if ( selidx < 0 || selidx >= selentries_.size() ) return;
    if ( cb ) recordEntryOffs();

    offsfld_->setValue( selentries_[selidx]->offs_ );
    curselidx_ = selidx;
}


void uiSeisMultiCubePS::addCube( CallBacker* )
{
    const int cubeidx = cubefld_->currentItem();
    if ( cubeidx < 0 ) return;
    recordEntryOffs();

    uiSeisMultiCubePSEntry* entry = entries_[cubeidx];
    entries_ -= entry;
    selentries_ += entry;

    curselidx_ = selentries_.size() - 1;
    fullUpdate();
}


void uiSeisMultiCubePS::rmCube( CallBacker* )
{
    const int selidx = selfld_->currentItem();
    if ( selidx < 0 ) return;

    uiSeisMultiCubePSEntry* entry = selentries_[selidx];
    selentries_ -= entry;
    entries_ += entry;

    if ( curselidx_ >= selentries_.size() )
	curselidx_ = selentries_.size() - 1;
    fullUpdate();
}


void uiSeisMultiCubePS::fillBox( uiListBox* lb )
{
    const ObjectSet<uiSeisMultiCubePSEntry>& es
		= lb == cubefld_ ? entries_ : selentries_;
    lb->empty();
    for ( int idx=0; idx<es.size(); idx++ )
	lb->addItem( es[idx]->ioobj_->name() );
}


void uiSeisMultiCubePS::fullUpdate()
{
    if ( selfld_->size() != selentries_.size() )
	fillBox( selfld_ );
    if ( cubefld_->size() != entries_.size() )
    {
	int cubeidx = cubefld_->currentItem();
	if ( cubeidx < 0 ) cubeidx = 0;
	fillBox( cubefld_ );
	if ( !cubefld_->isEmpty() )
	    cubefld_->setCurrentItem( cubeidx );
    }

    if ( curselidx_ >= 0 )
	offsfld_->setValue( selentries_[curselidx_]->offs_ );
    selfld_->setCurrentItem( curselidx_ );

}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisMultiCubePS::acceptOK( CallBacker* )
{
    if ( !outfld_->commitInput(true) )
	mErrRet("Please enter the output name")

    ObjectSet<MultiID> mids; TypeSet<float> offs;
    for ( int idx=0; idx<selentries_.size(); idx++ )
    {
	const uiSeisMultiCubePSEntry& entry = *selentries_[idx];
	mids += new MultiID( entry.ioobj_->key() );
	offs += entry.offs_;
    }

    BufferString emsg;
    bool ret = MultiCubeSeisPSReader::writeData(
			ctio_.ioobj->fullUserExpr(false), mids, offs, emsg );
    deepErase( mids );
    if ( !ret )
	mErrRet(emsg)

    return true;
}
