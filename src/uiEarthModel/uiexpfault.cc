/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiexpfault.cc,v 1.13 2009-04-27 04:40:31 cvsranojay Exp $";

#include "uiexpfault.h"

#include "bufstring.h"
#include "ctxtioobj.h"
#include "emfault3d.h"
#include "emfaultstickset.h"
#include "lmkemfaulttransl.h"
#include "emmanager.h"
#include "executor.h"
#include "filegen.h"
#include "filepath.h"
#include "ioobj.h"
#include "ptrman.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include <stdio.h>

#define mGet( tp, fss, f3d ) \
    !strcmp(tp,EMFaultStickSetTranslatorGroup::keyword()) ? fss : f3d

#define mGetCtio(tp) \
    mGet( tp, *mMkCtxtIOObj(EMFaultStickSet), *mMkCtxtIOObj(EMFault3D) )
#define mGetTitle(tp) \
    mGet( tp, "Export FaultStickSet", "Export Fault" )

uiExportFault::uiExportFault( uiParent* p, const char* typ )
    : uiDialog(p,uiDialog::Setup(mGetTitle(typ),
				 "Specify output format","104.1.1"))
    , ctio_(mGetCtio(typ))
    , linenmfld_(0)
{
    BufferString inplbl( "Input ");
    inplbl += typ;
    infld_ = new uiIOObjSel( this, ctio_, inplbl );

    coordfld_ = new uiGenInput( this, "Write coordinates as",
				BoolInpSpec(true,"X/Y","Inl/Crl") );
    coordfld_->attach( alignedBelow, infld_ );

    stickfld_ = new uiCheckBox( this, "stick index" );
    stickfld_->setChecked( true );
    stickfld_->attach( alignedBelow, coordfld_ );
    nodefld_ = new uiCheckBox( this, "node index" );
    nodefld_->setChecked( false );
    nodefld_->attach( rightTo, stickfld_ );
    uiLabel* lbl = new uiLabel( this, "Write", stickfld_ );

    if ( mGet(typ,true,false) )
    {
	linenmfld_ = new uiCheckBox( this, "Write line name if picked on 2D" );
	linenmfld_->setChecked( true );
	linenmfld_->attach( alignedBelow, stickfld_ );
    }

    outfld_ = new uiFileInput( this, "Output Ascii file",
	    		       uiFileInput::Setup().forread(false) );
    outfld_->setDefaultSelectionDir(
		IOObjContext::getDataDirName(IOObjContext::Surf) );
    outfld_->attach( alignedBelow, linenmfld_ ? linenmfld_ : stickfld_ );
}


uiExportFault::~uiExportFault()
{
    delete ctio_.ioobj; delete &ctio_;
}


static int nrSticks( EM::EMObject* emobj, EM::SectionID sid )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->sectionGeometry(sid));
    return fss->nrSticks();
}


static int nrKnots( EM::EMObject* emobj, EM::SectionID sid, int stickidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->sectionGeometry(sid));
    const int sticknr = fss->rowRange().atIndex( stickidx );
    return fss->nrKnots( sticknr );
}


static Coord3 getCoord( EM::EMObject* emobj, EM::SectionID sid, int stickidx,
			int knotidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->sectionGeometry(sid));
    const int sticknr = fss->rowRange().atIndex(stickidx);
    const int knotnr = fss->colRange(sticknr).atIndex(knotidx);
    return fss->getKnot( RowCol(sticknr,knotnr) );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportFault::writeAscii()
{
    const IOObj* ioobj = ctio_.ioobj;
    if ( !ioobj ) mErrRet("Cannot find fault in database");

    RefMan<EM::EMObject> emobj = EM::EMM().createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet("Cannot add fault to EarthModel")

    emobj->setMultiID( ioobj->key() );
    mDynamicCastGet(EM::Fault3D*,f3d,emobj.ptr())
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj.ptr())
    if ( !f3d && !fss ) return false;

    PtrMan<Executor> loader = emobj->loader();
    if ( !loader ) mErrRet("Cannot read fault")

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*loader) ) return false;

    const BufferString fname = outfld_->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( "Cannot open output file" );
    }

    BufferString str;
    const bool doxy = coordfld_->getBoolValue();
    const bool inclstickidx = stickfld_->isChecked();
    const bool inclknotidx = nodefld_->isChecked();
    const EM::SectionID sectionid = emobj->sectionID( 0 );
    const int nrsticks = nrSticks( emobj.ptr(), sectionid );
    for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
    {
	const int nrknots = nrKnots( emobj.ptr(), sectionid, stickidx );
	for ( int knotidx=0; knotidx<nrknots; knotidx++ )
	{
	    const Coord3 crd = getCoord( emobj.ptr(), sectionid,
		    			 stickidx, knotidx );
	    if ( !crd.isDefined() )
		continue;

	    if ( !doxy )
	    {
		const BinID bid = SI().transform( crd );
		*sdo.ostrm << bid.inl << '\t' << bid.crl;
	    }
	    else
	    {
		// ostreams print doubles awfully
		str.setEmpty();
		str += crd.x; str += "\t"; str += crd.y;
		*sdo.ostrm << str;
	    }

	    *sdo.ostrm << '\t' << crd.z*SI().zFactor();

	    if ( inclstickidx )
		*sdo.ostrm << '\t' << stickidx;
	    if ( inclknotidx )
		*sdo.ostrm << '\t' << knotidx;

	    if ( fss )
	    {
    		bool pickedon2d =
		    fss->geometry().pickedOn2DLine( sectionid, stickidx );
		if ( pickedon2d && linenmfld_->isChecked() )
		{
		    const char* linenm =
			fss->geometry().lineName( sectionid, stickidx );
		    *sdo.ostrm << '\t' << linenm;
		}
	    }

	    *sdo.ostrm << '\n';
	}
    }

    sdo.close();
    return true;
}


bool uiExportFault::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput() )
	mErrRet( "Please select the input fault" );
    if ( !strcmp(outfld_->fileName(),"") )
	mErrRet( "Please select output file" );

    if ( File_exists(outfld_->fileName()) && 
			!uiMSG().askOverwrite("Output file exists. Overwrite?") )
	return false;

    return writeAscii();
}
