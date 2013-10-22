/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          July 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelllogextract.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "keystrs.h"
#include "posinfo.h"
#include "posvecdataset.h"
#include "posfilterset.h"
#include "seisioobjinfo.h"
#include "wellextractdata.h"
#include "wellmarker.h"

#include "mousecursor.h"
#include "uiposfilterset.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uitaskrunner.h"
#include "unitofmeasure.h"


using namespace Attrib;

uiWellLogExtractGrp::uiWellLogExtractGrp( uiParent* p,
	const uiWellLogExtractGrp::Setup& setup, const Attrib::DescSet* d )
	: uiGroup(p)
	, ads_( d )
    	, attrsfld_(0)
    	, posfiltfld_(0)
    	, radiusfld_(0)
    	, curdps_(0)
    	, setup_(setup)
{
    welllogselfld_ =
	new uiMultiWellLogSel( this, uiWellExtractParams::Setup()
					.withsampling(true).withzstep(true)
					.withextractintime(SI().zIsTime())
					.singlelog(setup.singlelog_)
					.prefpropnm(setup.prefpropnm_));

    uiLabeledListBox* llba = 0;
    llba = new uiLabeledListBox( this, "Attributes", true );
    attrsfld_ = llba->box();
    llba->display( setup.withattrib_, true );
    welllogselfld_->attach( ensureBelow, llba );
    const float inldist = SI().inlDistance();
    const char* distunit =  SI().getXYUnitString();
    BufferString radiusbuf( "  Radius around wells "); 
    radiusbuf += distunit;
    radiusfld_ = new uiGenInput( this, radiusbuf,
				 FloatInpSpec((float)((int)(inldist+.5))) );
    if ( llba )
	radiusfld_->attach( alignedBelow, llba );
    else
	radiusfld_->attach( alignedBelow, welllogselfld_ );
    radiusfld_->attach( ensureBelow, welllogselfld_ );

    if ( ads_ && !ads_->is2D() )
    {
	uiPosFilterSet::Setup fsu( false );
	fsu.seltxt( "Filter positions" ).incprovs( true );
	posfiltfld_ = new uiPosFilterSetSel( this, fsu );
	posfiltfld_->attach( alignedBelow, radiusfld_ );
    }

    setDescSet( d );
}

#define mDPM DPM(DataPackMgr::PointID())

uiWellLogExtractGrp::~uiWellLogExtractGrp()
{
    releaseDPS();
}


void uiWellLogExtractGrp::releaseDPS()
{
    mDPM.release( curdps_ );
    curdps_ = 0;
}


const DataPointSet* uiWellLogExtractGrp::getDPS() const
{ return curdps_; }


void uiWellLogExtractGrp::setDescSet( const Attrib::DescSet* newads )
{
    ads_  = newads;
    adsChg();
    welllogselfld_->update();
}


void uiWellLogExtractGrp::adsChg()
{
    if ( !ads_ ) return;

    attrsfld_->setEmpty();

    Attrib::SelInfo attrinf( ads_, 0, ads_->is2D() );
    for ( int idx=0; idx<attrinf.attrnms_.size(); idx++ )
    {
	const Attrib::Desc* desc = ads_->getDesc( attrinf.attrids_[idx] );
	if ( desc && desc->is2D() && desc->isPS() )
	{
	    attrinf.attrnms_.removeSingle( idx );
	    attrinf.attrids_.removeSingle( idx );
	    idx--;
	    continue;
	}
	attrsfld_->addItem( attrinf.attrnms_.get(idx), false );
    }
    
    for ( int idx=0; idx<attrinf.ioobjids_.size(); idx++ )
    {
	BufferStringSet bss;
	SeisIOObjInfo sii( MultiID( attrinf.ioobjids_.get(idx) ) );
	if ( sii.isPS() && sii.is2D() )
	{
	    attrinf.ioobjids_.removeSingle( idx );
	    idx--;
	    continue;
	}
	sii.getDefKeys( bss, true );
	for ( int inm=0; inm<bss.size(); inm++ )
	{
	    const char* defkey = bss.get(inm).buf();
	    const char* ioobjnm = attrinf.ioobjnms_.get(idx).buf();
	    attrsfld_->addItem( attrinf.is2D(defkey)
		    ? SeisIOObjInfo::defKey2DispName(defkey,ioobjnm)
		    : SeisIOObjInfo::def3DDispName(defkey,ioobjnm) );
	}
    }
}


static void addDCDs( uiListBox* lb, ObjectSet<DataColDef>& dcds,
		     BufferStringSet& nms )
{
    for ( int idx=0; idx<lb->size(); idx++ )
    {
	if ( !lb->isSelected(idx) ) continue;
	const char* nm = lb->textOfItem(idx);
	nms.add( nm );
	dcds += new DataColDef( nm );
    }
}



#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiWellLogExtractGrp::extractWellData( const BufferStringSet& ioobjids,
					     const BufferStringSet& lognms,
					     ObjectSet<DataPointSet>& dpss )
{
    Well::TrackSampler wts( ioobjids, dpss, SI().zIsTime() );
    wts.for2d_ = false; wts.lognms_ = lognms;
    wts.locradius_ = !radiusfld_ ? 0.f : radiusfld_->getfValue();
    wts.mkdahcol_ = true;
    wts.params_ = welllogselfld_->params();
    uiTaskRunner tr( this );
    if ( !TaskRunner::execute( &tr, wts ) )
	return false;
    if ( dpss.isEmpty() )
	mErrRet("No wells found")
    bool founddata = false;
    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	if ( !dpss[idx]->isEmpty() )
	    { founddata = true; break; }
    }
    if ( !founddata )
    {
	if ( dpss.size() == 1 )
	    mErrRet("No valid data points found in the well\n"
		    "(check ranges, undefined sections ....)")
	else
	    mErrRet("No valid data points found in any of the wells\n"
		    "(check ranges, undefined sections ....)")
    }

    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	Well::LogDataExtracter wlde( ioobjids, dpss, SI().zIsTime() );
	wlde.lognm_ = lognms.get(idx);
	wlde.samppol_ = welllogselfld_->params().samppol_; 
	if ( !TaskRunner::execute( &tr, wlde ) )
	    return false;
    }

    return true;
}


bool uiWellLogExtractGrp::extractAttribData( DataPointSet& dps, int c1 )
{
    IOPar descsetpars;
    ads_->fillPar( descsetpars );
    const_cast<PosVecDataSet*>( &(dps.dataSet()) )->pars() = descsetpars;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    Attrib::EngineMan aem; BufferString errmsg;
    PtrMan<Executor> tabextr = aem.getTableExtractor( dps, *ads_, errmsg, c1 );
    MouseCursorManager::restoreOverride();
    if ( !errmsg.isEmpty() )
	mErrRet(errmsg)
    uiTaskRunner tr( this );
    return TaskRunner::execute( &tr, *tabextr );
}


void uiWellLogExtractGrp::getWellNames( BufferStringSet& wellnms )
{ welllogselfld_->getSelWellNames( wellnms ); }

void uiWellLogExtractGrp::getSelLogNames( BufferStringSet& lognms )
{ welllogselfld_->getSelLogNames( lognms ); }

#undef mErrRet
#define mErrRet(s) { deepErase(dcds); if ( s ) uiMSG().error(s); return false; }

bool uiWellLogExtractGrp::extractDPS()
{
    ObjectSet<DataColDef> dcds;
    BufferString unit( "MD" );
    unit.add( UnitOfMeasure::zUnitAnnot(false,true,true) );
    dcds += new DataColDef( unit );
    BufferStringSet lognms; welllogselfld_->getSelLogNames( lognms );
    for ( int idx=0; idx<lognms.size(); idx++ )
	dcds += new DataColDef( lognms[idx]->buf() );
    if ( lognms.isEmpty() )
	mErrRet("Please select at least one log")
    BufferStringSet attrnms;
    if ( ads_ )
	addDCDs( attrsfld_, dcds,  attrnms );

    BufferStringSet ioobjids, wellnms;
    welllogselfld_->getSelWellNames( wellnms );
    welllogselfld_->getSelWellIDs( ioobjids );
    if ( ioobjids.isEmpty() )
	mErrRet("Please select at least one well")

    ObjectSet<DataPointSet> dpss;
    if ( !extractWellData(ioobjids,lognms,dpss) )
	mErrRet(0)

    PtrMan<Pos::Filter> filt = 0;
    if ( posfiltfld_ )
    {
	IOPar iop; posfiltfld_->fillPar( iop );
	filt = Pos::Filter::make( iop, false );
	if ( filt )
	{
	    uiTaskRunner tr( this );
	    if ( !filt->initialize(&tr) )
		return false;
	}
    }

    MouseCursorManager::setOverride( MouseCursor::Wait );
    if ( curdps_ )
	releaseDPS();
    curdps_ =
	new DataPointSet( TypeSet<DataPointSet::DataRow>(), dcds, false, false);
    mDPM.addAndObtain( curdps_ );

    const UnitOfMeasure* uom = UoMR().get( "Feet" );
    deepErase( dcds );
    const int nrattribs = attrnms.size();
    const int nrlogs = lognms.size() + 1;
    DataPointSet::DataRow dr;
    for ( int idps=0; idps<dpss.size(); idps++ )
    {
	DataPointSet& curdps = *dpss[idps];
	for ( int idr=0; idr<curdps.size(); idr++ )
	{
	    dr = curdps.dataRow( idr );
	    if ( filt && !filt->includes(dr.pos_.coord(),dr.pos_.z()) )
		continue;

	    DataPointSet::DataRow newdr( dr );
	    newdr.data_.setSize( nrattribs + nrlogs, mUdf(float) );
	    for ( int ilog=0; ilog<nrlogs; ilog++ )
		newdr.data_[ilog] = dr.data_[ilog];
	    for ( int iattr=0; iattr<nrattribs; iattr++ )
		newdr.data_[nrlogs+iattr] = mUdf(float);
	    newdr.setGroup( (unsigned short)(idps+1) );

	    if ( uom && !SI().zInFeet() && SI().depthsInFeet() )
		newdr.data_[0] = uom->getUserValueFromSI( newdr.data_[0] );

	    curdps_->setRow( newdr );
	}
    }

    deepErase( dpss );
    curdps_->dataChanged();
    MouseCursorManager::restoreOverride();
    if ( curdps_->isEmpty() )
	mErrRet("No positions found matching criteria")

    BufferString dpsnm( "Well data:" );
    for ( int idx=0; idx<wellnms.size(); idx++ )
    { dpsnm += wellnms[idx]->buf(); if ( idx!=wellnms.size()-1 ) dpsnm += ","; }

    if ( !attrnms.isEmpty() )
    {
	dpsnm += " / Attributes";
	if ( !extractAttribData(*curdps_,nrlogs) )
	    return false;
    }

    curdps_->setName( dpsnm );
    return true;
}
