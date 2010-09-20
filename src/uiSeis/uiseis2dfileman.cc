/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseis2dfileman.cc,v 1.10 2010-09-20 09:01:00 cvssatyaki Exp $";


#include "uiseis2dfileman.h"
#include "uiseispsman.h"

#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "pixmap.h"
#include "seis2dline.h"
#include "seis2dlinemerge.h"
#include "seiscube2linedata.h"
#include "survinfo.h"
#include "zdomain.h"
#include "linesetposinfo.h"

#include "uibutton.h"
#include "uigeninputdlg.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseisbrowser.h"
#include "uiseissel.h"
#include "uisplitter.h"
#include "uitextedit.h"
#include "uitaskrunner.h"


Notifier<uiSeis2DFileMan>* uiSeis2DFileMan::fieldsCreated()
{
    static Notifier<uiSeis2DFileMan> FieldsCreated(0);
    return &FieldsCreated;
}


uiSeis2DFileMan::uiSeis2DFileMan( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,uiDialog::Setup("Seismic line data management",
				 "Manage 2D seismic lines",
				 "103.1.3"))
    , issidomain(ZDomain::isSI( ioobj.pars() ))
    , zistm((SI().zIsTime() && issidomain) || (!SI().zIsTime() && !issidomain))
{
    setCtrlStyle( LeaveOnly );

    objinfo_ = new uiSeisIOObjInfo( ioobj );
    lineset_ = new Seis2DLineSet( ioobj.fullUserExpr(true) );

    uiGroup* topgrp = new uiGroup( this, "Top" );
    uiLabeledListBox* lllb = new uiLabeledListBox( topgrp, "2D lines", false,
						  uiLabeledListBox::AboveMid );
    linefld_ = lllb->box();
    linefld_->selectionChanged.notify( mCB(this,uiSeis2DFileMan,lineSel) );
    linefld_->setMultiSelect(true);

    linegrp_ = new uiManipButGrp( lllb );
    linegrp_->addButton( uiManipButGrp::Rename, 
		 mCB(this,uiSeis2DFileMan,renameLine), "Rename line");
    linegrp_->addButton( ioPixmap("mergelines.png"),
		 mCB(this,uiSeis2DFileMan,mergeLines), "Merge lines");
    if ( SI().has3D() )
	linegrp_->addButton( ioPixmap("extr3dseisinto2d.png"),
		 mCB(this,uiSeis2DFileMan,extrFrom3D), "Extract from 3D cube");
    linegrp_->attach( rightOf, linefld_ );

    uiLabeledListBox* allb = new uiLabeledListBox( topgrp, "Attributes", true,
						   uiLabeledListBox::AboveMid );
    attrfld_ = allb->box();
    attrfld_->selectionChanged.notify( mCB(this,uiSeis2DFileMan,attribSel) );
    allb->attach( rightOf, lllb );

    attrgrp_ = new uiManipButGrp( allb );
    attrgrp_->addButton( uiManipButGrp::Rename,
	    	       mCB(this,uiSeis2DFileMan,renameAttrib),
		       "Rename attribute" );
    attrgrp_->addButton( uiManipButGrp::Remove,
	    		mCB(this,uiSeis2DFileMan,removeAttrib),
			"Remove selected attribute(s)" );
    browsebut_ = attrgrp_->addButton( ioPixmap("browseseis.png"),
	    	       mCB(this,uiSeis2DFileMan,browsePush),
		       "Browse/edit this line" );
    attrgrp_->attach( rightOf, attrfld_ );

    uiGroup* botgrp = new uiGroup( this, "Bottom" );
    infofld_ = new uiTextEdit( botgrp, "File Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setPrefWidthInChar( 50 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    fillLineBox();

    fieldsCreated()->trigger( this );
    lineSel(0);
}


uiSeis2DFileMan::~uiSeis2DFileMan()
{
    delete objinfo_;
    delete lineset_;
}


void uiSeis2DFileMan::fillLineBox()
{
    uiListBox* lb = linefld_;
    const int curitm = lb->size() ? lb->currentItem() : 0;
    BufferStringSet linenames;
    objinfo_->ioObjInfo().getLineNames( linenames );
    lb->empty();
    lb->addItems( linenames );
    lb->setSelected( curitm );
}


void uiSeis2DFileMan::lineSel( CallBacker* )
{
    BufferStringSet sellines;
    linefld_->getSelectedItems( sellines );
    BufferStringSet sharedattribs;
    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	SeisIOObjInfo::Opts2D opts2d; opts2d.zdomky_ = "*";
	BufferStringSet attrs;
	objinfo_->ioObjInfo().
	    getAttribNamesForLine( sellines.get(idx), attrs, opts2d );
	if ( !idx )
	{
	    sharedattribs = attrs;
	    continue;
	}

	BufferStringSet strs2rem;
	for ( int ida=0; ida<sharedattribs.size(); ida++ )
	{
	    const char* str = sharedattribs.get(ida);
	    int index = attrs.indexOf( str );
	    if ( index<0 ) strs2rem.add( str );
	}

	for ( int ida=0; ida<strs2rem.size(); ida++ )
	{
	    const int index = sharedattribs.indexOf( strs2rem.get(ida) );
	    sharedattribs.remove( index );
	}
    }

    attrfld_->empty();
    sharedattribs.sort();
    attrfld_->addItems( sharedattribs );
    attrfld_->setSelected( 0, true );
}


void uiSeis2DFileMan::attribSel( CallBacker* )
{
    infofld_->setText( "" );
    BufferStringSet linenms, attribnms;
    linefld_->getSelectedItems( linenms );
    attrfld_->getSelectedItems( attribnms );
    if ( linenms.isEmpty() || attribnms.isEmpty() )
	{ browsebut_->setSensitive( false ); return; }

    const LineKey linekey( linenms.get(0), attribnms.get(0) );
    const int lineidx = lineset_->indexOf( linekey );
    if ( lineidx < 0 ) { pErrMsg("Huh"); return; }

    PosInfo::Line2DData l2dd;
    if ( !lineset_->getGeometry(lineidx,l2dd) || l2dd.isEmpty() )
	return;

#define mAddZRangeTxt(memb) txt += zistm ? mNINT(1000*memb) : memb

    const TypeSet<PosInfo::Line2DPos>& posns = l2dd.positions();
    const int sz = posns.size();
    BufferString txt( "Number of traces: " ); txt += sz;
    const PosInfo::Line2DPos& firstpos = posns[0];
    txt += "\nFirst trace: "; txt += firstpos.nr_;
    txt += " ("; txt += firstpos.coord_.x;
    txt += ","; txt += firstpos.coord_.y; txt += ")";
    const PosInfo::Line2DPos& lastpos = posns[sz-1];
    txt += "\nLast trace: "; txt += lastpos.nr_;
    txt += " ("; txt += lastpos.coord_.x;
    txt += ","; txt += lastpos.coord_.y; txt += ")";
    txt += "\nZ-range: "; mAddZRangeTxt(l2dd.zRange().start); txt += " - ";
    mAddZRangeTxt(l2dd.zRange().stop);
    txt += " ["; mAddZRangeTxt(l2dd.zRange().step); txt += "]";

    SeisIOObjInfo sobinf( objinfo_->ioObj() );
    const int nrcomp = sobinf.nrComponents( linekey );
    if ( nrcomp > 1 )
	{ txt += "\nNumber of components: "; txt += nrcomp; }

    const IOPar& iopar = lineset_->getInfo( lineidx );
    BufferString fname(iopar.find(sKey::FileName) );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
	fp.setPath( IOObjContext::getDataDirName(IOObjContext::Seis) );
    fname = fp.fullPath();

    txt += "\nLocation: "; txt += fp.pathOnly();
    txt += "\nFile name: "; txt += fp.fileName();
    txt += "\nFile size: "; 
    txt += uiObjFileMan::getFileSizeString( File::getKbSize(fname) );
    const char* timestr = File::timeLastModified( fname );
    if ( timestr ) { txt += "\nLast modified: "; txt += timestr; }
    infofld_->setText( txt );

    browsebut_->setSensitive( true );
}


void uiSeis2DFileMan::browsePush( CallBacker* )
{
    if ( !objinfo_ || !objinfo_->ioObj() ) return;
    const LineKey lk( linefld_->getText(), attrfld_->getText());
    uiSeisBrowser::doBrowse( this, *objinfo_->ioObj(), true, &lk );
}


void uiSeis2DFileMan::removeAttrib( CallBacker* )
{
    BufferStringSet attribnms;
    attrfld_->getSelectedItems( attribnms );
    if ( attribnms.isEmpty()
      || !uiMSG().askRemove("All selected attributes will be removed.\n"
			     "Do you want to continue?") )
	return;

    BufferStringSet sellines;
    linefld_->getSelectedItems( sellines );
    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	const char* linename = sellines.get(idx);
	for ( int ida=0; ida<attribnms.size(); ida++ )
	{
	    LineKey linekey( linename, attribnms.get(ida) );
	    if ( !lineset_->remove(linekey) )
		uiMSG().error( "Could not remove attribute" );
	}
    }

    fillLineBox();
}


bool uiSeis2DFileMan::rename( const char* oldnm, BufferString& newnm )
{
    BufferString titl( "Rename '" );
    titl += oldnm; titl += "'";
    uiGenInputDlg dlg( this, titl, "New name", new StringInpSpec(oldnm) );
    if ( !dlg.go() ) return false;
    newnm = dlg.text();
    return newnm != oldnm;
}


void uiSeis2DFileMan::renameLine( CallBacker* )
{
    BufferStringSet linenms;
    linefld_->getSelectedItems( linenms );
    if ( linenms.isEmpty() ) return;

    const char* linenm = linenms.get(0);
    BufferString newnm;
    if ( !rename(linenm,newnm) ) return;
    
    if ( linefld_->isPresent(newnm) )
    {
	uiMSG().error( "Linename already in use" );
	return;
    }

    if ( !lineset_->renameLine( linenm, newnm ) )
    {
	uiMSG().error( "Could not rename line" );
	return;
    }

    fillLineBox();
}


void uiSeis2DFileMan::renameAttrib( CallBacker* )
{
    BufferStringSet attribnms;
    attrfld_->getSelectedItems( attribnms );
    if ( attribnms.isEmpty() ) return;

    const char* attribnm = attribnms.get(0);
    BufferString newnm;
    if ( !rename(attribnm,newnm) ) return;

    if ( attrfld_->isPresent(newnm) )
    {
	uiMSG().error( "Attribute name already in use" );
	return;
    }

    BufferStringSet sellines;
    linefld_->getSelectedItems( sellines );
    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	const char* linenm = sellines.get(idx);
	LineKey oldlk( linenm, attribnm );
	if ( !lineset_->rename(oldlk,LineKey(linenm,newnm)) )
	{
	    BufferString err( "Could not rename attribute: " );
	    err += oldlk;
	    uiMSG().error( err );
	    continue;
	}
    }

    lineSel(0);
}


class uiSeis2DFileManMergeDlg : public uiDialog
{
public:

uiSeis2DFileManMergeDlg( uiParent* p, const uiSeisIOObjInfo& objinf,
			 const BufferStringSet& sellns )
    : uiDialog(p,Setup("Merge lines","Merge two lines into a new one",
		       "103.1.9") )
    , objinf_(objinf)
{
    BufferStringSet lnms; objinf_.ioObjInfo().getLineNames( lnms );
    uiLabeledComboBox* lcb1 = new uiLabeledComboBox( this, lnms, "First line" );
    uiLabeledComboBox* lcb2 = new uiLabeledComboBox( this, lnms, "Add" );
    lcb2->attach( alignedBelow, lcb1 );
    ln1fld_ = lcb1->box(); ln2fld_ = lcb2->box();
    ln1fld_->setCurrentItem( sellns.get(0) );
    ln2fld_->setCurrentItem( sellns.get(1) );

    static const char* mrgopts[]
	= { "Match trace numbers", "Match coordinates", "Bluntly append", 0 };
    mrgoptfld_ = new uiGenInput( this, "Merge method",
	    			 StringListInpSpec(mrgopts) );
    mrgoptfld_->attach( alignedBelow, lcb2 );
    mrgoptfld_->valuechanged.notify( mCB(this,uiSeis2DFileManMergeDlg,optSel) );
    stckfld_ = new uiGenInput( this, "Duplicate positions",
	    			BoolInpSpec(true,"Stack","Use first") );
    stckfld_->attach( alignedBelow, mrgoptfld_ );
    renumbfld_ = new uiGenInput( this, "Renumber; Start/step numbers",
	    			 IntInpSpec(1), IntInpSpec(1) );
    renumbfld_->setWithCheck( true );
    renumbfld_->setChecked( true );
    renumbfld_->attach( alignedBelow, stckfld_ );
    double defsd = SI().crlDistance() / 2;
    if ( SI().xyInFeet() ) defsd *= mToFeetFactorD;
    snapdistfld_ = new uiGenInput( this, "Snap distance", DoubleInpSpec(defsd));
    snapdistfld_->attach( alignedBelow, renumbfld_ );

    outfld_ = new uiGenInput( this, "New line name", StringInpSpec() );
    outfld_->attach( alignedBelow, snapdistfld_ );

    finaliseDone.notify( mCB(this,uiSeis2DFileManMergeDlg,initWin) );
}

void initWin( CallBacker* )
{
    optSel(0);
    renumbfld_->valuechanged.notify( mCB(this,uiSeis2DFileManMergeDlg,optSel) );
    renumbfld_->checked.notify( mCB(this,uiSeis2DFileManMergeDlg,optSel) );
}

void optSel( CallBacker* )
{
    const int opt = mrgoptfld_->getIntValue();
    stckfld_->display( opt < 2 );
    renumbfld_->display( opt > 0 );
    snapdistfld_->display( opt == 1 );
}

#define mErrRet(s) { uiMSG().error(s); return false; }
bool acceptOK( CallBacker* )
{
    const char* outnm = outfld_->text();
    if ( !outnm || !*outnm )
	mErrRet( "Please enter a name for the merged line" );

    BufferStringSet lnms; objinf_.ioObjInfo().getLineNames( lnms );
    if ( lnms.isPresent( outnm ) )
	mErrRet( "Output line name already in Line Set" );

    Seis2DLineMerger lmrgr( objinf_.ioObj()->key() );
    lmrgr.lnm1_ = ln1fld_->text();
    lmrgr.lnm2_ = ln2fld_->text();
    if ( lmrgr.lnm1_ == lmrgr.lnm2_ )
	mErrRet( "Respectfully refusing to merge a line with itself" );

    lmrgr.outlnm_ = outnm;
    lmrgr.opt_ = (Seis2DLineMerger::Opt)mrgoptfld_->getIntValue();
    lmrgr.stckdupl_ = stckfld_->getBoolValue();
    lmrgr.renumber_ = lmrgr.opt_ != Seis2DLineMerger::MatchTrcNr
		   && renumbfld_->isChecked();

    if ( lmrgr.renumber_ )
    {
	lmrgr.numbering_.start = renumbfld_->getIntValue(0);
	lmrgr.numbering_.step = renumbfld_->getIntValue(1);
    }

    if ( lmrgr.opt_ == Seis2DLineMerger::MatchCoords )
    {
	lmrgr.snapdist_ = snapdistfld_->getdValue();
	if ( mIsUdf(lmrgr.snapdist_) || lmrgr.snapdist_ < 0 )
	    mErrRet( "Please specify a valid snap distance" );
    }

    uiTaskRunner tr( this );
    tr.execute( lmrgr );
    // return tr.execute( lmrgr );
    return false;
}

    const uiSeisIOObjInfo&	objinf_;

    uiComboBox*			ln1fld_;
    uiComboBox*			ln2fld_;
    uiGenInput*			mrgoptfld_;
    uiGenInput*			stckfld_;
    uiGenInput*			renumbfld_;
    uiGenInput*			snapdistfld_;
    uiGenInput*			outfld_;

};


void uiSeis2DFileMan::redoAllLists()
{
    const MultiID lsid( objinfo_->ioObj()->key() );
    delete objinfo_;
    objinfo_ = new uiSeisIOObjInfo( lsid );
    if ( objinfo_->isOK() )
    {
	delete lineset_;
	lineset_ = new Seis2DLineSet( objinfo_->ioObj()->fullUserExpr(true) );
    }
    fillLineBox();
}


void uiSeis2DFileMan::mergeLines( CallBacker* )
{
    if ( linefld_->size() < 2 ) return;

    BufferStringSet sellnms; int firstsel = -1;
    for ( int idx=0; idx<linefld_->size(); idx++ )
    {
	if ( linefld_->isSelected(idx) )
	{
	    sellnms.add( linefld_->textOfItem(idx) );
	    if ( firstsel < 0 ) firstsel = idx;
	    if ( sellnms.size() > 1 ) break;
	}
    }
    if ( firstsel < 0 )
	{ firstsel = 0; sellnms.add( linefld_->textOfItem(0) ); }
    if ( sellnms.size() == 1 )
    {
	if ( firstsel >= linefld_->size() )
	    firstsel = -1;
	sellnms.add( linefld_->textOfItem(firstsel+1) );
    }

    uiSeis2DFileManMergeDlg dlg( this, *objinfo_, sellnms );
    if ( dlg.go() )
	redoAllLists();
}


class uiSeis2DExtractFrom3D : public uiDialog
{
public:

uiSeis2DExtractFrom3D( uiParent* p, const uiSeisIOObjInfo& objinf,
			 const BufferStringSet& sellns )
    : uiDialog(p,Setup("Extract from 3D","Get 3D data as 2D line attribute",
		       "103.1.10") )
    , objinf_(objinf)
    , sellns_(sellns)
{
    alllnsfld_ = new uiGenInput( this, "Extract for",
	    		BoolInpSpec(true,"All lines", "Selected line(s)") );
    cubefld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,true),
	    			uiSeisSel::Setup(Seis::Vol) );
    cubefld_->attach( alignedBelow, alllnsfld_ );
    cubefld_->selectionDone.notify( mCB(this,uiSeis2DExtractFrom3D,cubeSel) );
    attrnmfld_ = new uiGenInput( this, "Store as attribute" );
    attrnmfld_->attach( alignedBelow, cubefld_ );
}

void cubeSel( CallBacker* )
{
    const IOObj* ioobj = cubefld_->ioobj( true );
    const char* attrnm = attrnmfld_->text();
    if ( ioobj && !*attrnm )
	attrnmfld_->setText( ioobj->name() );
}

bool acceptOK( CallBacker* )
{
    const IOObj* ioobj = cubefld_->ioobj();
    if ( !ioobj ) return false;
    const char* attrnm = attrnmfld_->text();
    if ( !*attrnm )
	{ uiMSG().error("Please provide an attribute name"); return false; }

    BufferStringSet lnms;
    if ( !alllnsfld_->getBoolValue() )
	lnms = sellns_;

    SeisCube2LineDataExtracter extr( *ioobj, *objinf_.ioObj(), attrnm,
	    			     lnms.isEmpty() ? 0 : &lnms );
    uiTaskRunner tr( this );
    return tr.execute( extr );
}

    const uiSeisIOObjInfo&	objinf_;
    const BufferStringSet&	sellns_;

    uiSeisSel*		cubefld_;
    uiGenInput*		alllnsfld_;
    uiGenInput*		attrnmfld_;

};


void uiSeis2DFileMan::extrFrom3D( CallBacker* )
{
    if ( linefld_->size() < 1 ) return;

    BufferStringSet sellnms; linefld_->getSelectedItems( sellnms );
    uiSeis2DExtractFrom3D dlg( this, *objinfo_, sellnms );
    if ( dlg.go() )
	redoAllLists();
}
