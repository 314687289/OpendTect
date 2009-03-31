/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiconvpos.cc,v 1.31 2009-03-31 04:49:43 cvsnanne Exp $";

#include "uiconvpos.h"
#include "pixmap.h"
#include "survinfo.h"
#include "strmprov.h"
#include "oddirs.h"
#include "uibutton.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uimsg.h"
#include <iostream>

#define mMaxLineBuf 32000
static BufferString lastinpfile;
static BufferString lastoutfile;


uiConvertPos::uiConvertPos( uiParent* p, const SurveyInfo& si, bool mod )
	: uiDialog(p, uiDialog::Setup("Position conversion",
		   "Coordinates vs Inline/Crossline","0.3.7").modal(mod))
	, survinfo(si)
{
    ismanfld = new uiGenInput( this, "Conversion",
	           BoolInpSpec(true,"Manual","File") );
    ismanfld->valuechanged.notify( mCB(this,uiConvertPos,selChg) );

    mangrp = new uiGroup( this, "Manual group" );
    uiGroup* inlcrlgrp = new uiGroup( mangrp, "InlCrl group" );
    inlfld = new uiGenInput( inlcrlgrp, "In-line", 
			     IntInpSpec().setName("Inl-field") );
    inlfld->setElemSzPol( uiObject::Small );
    crlfld = new uiGenInput( inlcrlgrp, "Cross-line", 
	    		     IntInpSpec().setName("Crl-field") );
    crlfld->setElemSzPol( uiObject::Small );
    crlfld->attach( alignedBelow, inlfld );

    uiGroup* xygrp = new uiGroup( mangrp, "XY group" );
    xfld = new uiGenInput( xygrp, "X-coordinate",
			   DoubleInpSpec().setName("X-field") );
    xfld->setElemSzPol( uiObject::Small );
    yfld = new uiGenInput( xygrp, "Y-coordinate", 
	    		   DoubleInpSpec().setName("Y-field") );
    yfld->setElemSzPol( uiObject::Small );
    yfld->attach( alignedBelow, xfld );

    uiGroup* butgrp = new uiGroup( mangrp, "Buttons" );
    const ioPixmap right( "forward.xpm" ); const ioPixmap left( "back.xpm" );
    uiToolButton* dobinidbut = new uiToolButton( butgrp, "Left", left );
    dobinidbut->activated.notify( mCB(this,uiConvertPos,getBinID) );
    dobinidbut->setToolTip( "Convert (X,Y) to Inl/Crl" );
    uiToolButton* docoordbut = new uiToolButton( butgrp, "Right", right );
    docoordbut->activated.notify( mCB(this,uiConvertPos,getCoord) );
    docoordbut->attach( rightTo, dobinidbut );
    docoordbut->setToolTip( "Convert Inl/Crl to (X,Y)" );
    butgrp->attach( centeredRightOf, inlcrlgrp );
    xygrp->attach( centeredRightOf, butgrp );

    mangrp->setHAlignObj( inlfld );
    mangrp->attach( alignedBelow, ismanfld );

    filegrp = new uiGroup( this, "File group" );
    uiFileInput::Setup fipsetup( lastinpfile );
    fipsetup.forread(true).withexamine(true).examinetablestyle(true)
	    .defseldir(GetDataDir());
    inpfilefld = new uiFileInput( filegrp, "Input file", fipsetup );

    fipsetup.fnm = lastoutfile;
    fipsetup.forread(false).withexamine(false);
    outfilefld = new uiFileInput( filegrp, "Output file", fipsetup );
    outfilefld->attach( alignedBelow, inpfilefld );
    isxy2bidfld = new uiGenInput( filegrp, "Type",
	           BoolInpSpec(true,"X/Y to I/C","I/C to X/Y") );
    isxy2bidfld->attach( alignedBelow, outfilefld );
    uiPushButton* pb = new uiPushButton( filegrp, "Go",
	    			mCB(this,uiConvertPos,convFile), true );
    pb->attach( alignedBelow, isxy2bidfld );
    filegrp->setHAlignObj( inpfilefld );
    filegrp->attach( alignedBelow, ismanfld );

    setCtrlStyle( LeaveOnly );
    finaliseDone.notify( mCB(this,uiConvertPos,selChg) );
}


void uiConvertPos::selChg( CallBacker* )
{
    const bool isman = ismanfld->getBoolValue();
    mangrp->display( isman );
    filegrp->display( !isman );
}


void uiConvertPos::getCoord( CallBacker* )
{
    BinID binid( inlfld->getIntValue(), crlfld->getIntValue() );
    if ( binid == BinID::udf() )
    {
	uiMSG().error( "Cannot convert this position" );
	xfld->setText( "" ); yfld->setText( "" );
	return;
    }

    Coord coord( survinfo.transform( binid ) );
    xfld->setValue( coord.x );
    yfld->setValue( coord.y );
    inlfld->setValue( binid.inl );
    crlfld->setValue( binid.crl );
}


void uiConvertPos::getBinID( CallBacker* )
{
    Coord coord( xfld->getdValue(), yfld->getdValue() );
    if ( coord == Coord::udf() )
    {
	uiMSG().error( "Cannot convert this position" );
	inlfld->setText( "" ); crlfld->setText( "" );
	return;
    }

    BinID binid( survinfo.transform( coord ) );
    inlfld->setValue( binid.inl );
    crlfld->setValue( binid.crl );
    xfld->setValue( coord.x );
    yfld->setValue( coord.y );
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiConvertPos::convFile( CallBacker* )
{
    const BufferString inpfnm = inpfilefld->fileName();
    StreamData sdin = StreamProvider(inpfnm).makeIStream();
    if ( !sdin.usable() )
	mErrRet("Input file is not readable" );

    const BufferString outfnm = outfilefld->fileName();
    StreamData sdout = StreamProvider(outfnm).makeOStream();
    if ( !sdout.usable() )
	{ sdin.close(); mErrRet("Cannot open output file" ); }

    lastinpfile = inpfnm; lastoutfile = outfnm;

    char linebuf[mMaxLineBuf]; Coord c;
    const bool xy2ic = isxy2bidfld->getBoolValue();
    int nrln = 0;
    while ( *sdin.istrm )
    {
	*sdin.istrm >> c.x;
	if ( sdin.istrm->eof() ) break;
	*sdin.istrm >> c.y;
	sdin.istrm->getline( linebuf, mMaxLineBuf );
	if ( sdin.istrm->bad() ) break;
	if ( xy2ic )
	{
	    BinID bid( SI().transform(c) );
	    *sdout.ostrm << bid.inl << ' ' << bid.crl << linebuf << '\n';
	}
	else
	{
	    BinID bid( mNINT(c.x), mNINT(c.y) );
	    c = SI().transform( bid );
	    *sdout.ostrm << getStringFromDouble(0,c.x) << ' ';
	    *sdout.ostrm << getStringFromDouble(0,c.y) << linebuf << '\n';
	}
	nrln++;
    }

    sdin.close(); sdout.close();
    uiMSG().message( "Total number of converted lines: ",
	    	     getStringFromInt(nrln) );
}


