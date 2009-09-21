/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltgen.cc,v 1.8 2009-09-21 11:23:27 cvsbruno Exp $";


#include "uiseiswvltgen.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "fft.h"
#include "ioobj.h"
#include "iodirentry.h"
#include "ioman.h"
#include "interpol1d.h"
#include "mathfunc.h"
#include "survinfo.h"
#include "wavelet.h"

#include "uiioobjsel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiwindowfuncseldlg.h"
#include "uiseiswvltattr.h"
#include "uiworld2ui.h"
#include "uimsg.h"


uiSeisWvltCreate::uiSeisWvltCreate( uiParent* p, uiDialog::Setup su ) 
	: uiDialog(p,su)
	, ctio_(*mMkCtxtIOObj(Wavelet))
{
    ctio_.ctxt.forread = false;
    wvltfld_ = new uiIOObjSel( this, ctio_ );
}
	

uiSeisWvltCreate::~uiSeisWvltCreate()
{
    delete ctio_.ioobj; delete &ctio_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisWvltCreate::putWvlt( const Wavelet& wvlt )
{
    if ( !wvltfld_->commitInput() )
	mErrRet( "Please enter a name for the new Wavelet" );

    if ( !wvlt.put(ctio_.ioobj) )
	mErrRet( "Cannot write wavelet" )

    return true;
}


MultiID uiSeisWvltCreate::storeKey() const
{
    return ctio_.ioobj ? ctio_.ioobj->key() : MultiID("");
}




uiSeisWvltGen::uiSeisWvltGen( uiParent* p )
    : uiSeisWvltCreate(p,uiDialog::Setup("Create Wavelet",
				 "Specify wavelet creation parameters",
				 "103.3.2"))
{
    isrickfld_ = new uiGenInput( this, "Wavelet type",
				BoolInpSpec(true,"Ricker","Sinc") );

    const float sisr = SI().zStep();
    float deffrq = 0.1 / sisr; int ideffr = mNINT(deffrq);
    if ( ideffr > 0 && mIsZero(deffrq-ideffr,1e-4) )
	deffrq = ideffr; // avoid awkward 99.999 display
    BufferString txt( "Central " );
    txt += SI().zIsTime() ? "Frequency" : "Wavenumber";
    freqfld_ = new uiGenInput( this, txt, FloatInpSpec(deffrq) );
    freqfld_->attach( alignedBelow, isrickfld_ );

    const float usrsr = sisr * SI().zFactor();
    txt = "Sample interval "; txt += SI().getZUnitString();
    srfld_ = new uiGenInput( this, txt, FloatInpSpec(usrsr) );
    srfld_->attach( alignedBelow, freqfld_ );

    peakamplfld_ = new uiGenInput( this, "Peak amplitude", FloatInpSpec(1) );
    peakamplfld_->attach( alignedBelow, srfld_ );
   
    wvltfld_->attach( alignedBelow, peakamplfld_ );
}


bool uiSeisWvltGen::acceptOK( CallBacker* )
{
    const float sr = srfld_->getfValue();
    const float peakampl = peakamplfld_->getfValue();
    const float freq = freqfld_->getfValue();

    if ( mIsUdf(sr) || sr <= 0 )
	mErrRet( "The sample interval is not valid" )
    else if ( peakampl == 0 )
	mErrRet( "The peak amplitude must be non-zero" )
    else if ( mIsUdf(freq) || freq <= 0 )
	mErrRet( "The frequency must be positive" )

    const float realsr = sr / SI().zFactor();
    Wavelet wvlt( isrickfld_->getBoolValue(), freq, realsr, peakampl );
    putWvlt( wvlt );

    return true;
}


static const char* centernms[] = { "maximum amplitude", "maximum Energy", 0 };
uiSeisWvltMerge::uiSeisWvltMerge( uiParent* p, const char* curwvltnm )
    : uiSeisWvltCreate(p,uiDialog::Setup("Merge Wavelets",
				 "Select two ore more wavelets to be stacked",
				 mTODOHelpID))
    , maxwvltsize_(0)					      
    , stackedwvlt_(0)					      
    , curwvltnm_(curwvltnm)
{
    normalizefld_ = new uiCheckBox( this, "Normalize wavelets" );
    normalizefld_->activated.notify( mCB(this,uiSeisWvltMerge,reloadAll) );
    centerfld_ = new uiCheckBox( this, "Center wavelets" );
    centerfld_->activated.notify( mCB(this,uiSeisWvltMerge,centerChged) );
    centerfld_->activated.notify( mCB(this,uiSeisWvltMerge,reloadAll) );
    centerfld_->attach( rightOf, normalizefld_ );
    centerchoicefld_ = new uiLabeledComboBox( this, "at" );
    centerchoicefld_->box()->addItems( centernms );
    centerchoicefld_->box()->selectionChanged.notify( 
	    				mCB(this,uiSeisWvltMerge,reloadAll) );
    centerchoicefld_->attach( rightOf, centerfld_ );
    centerchoicefld_->display( false );

    reloadWvlts();
    constructDrawer( false );
    constructDrawer( true );
    reloadFunctions();

    wvltfld_->setLabelText("Save stacked wavelet");
    wvltfld_->setSensitive( false );

    for ( int idx=0; idx<wvltdrawer_.size(); idx++ )
    {
	wvltdrawer_[idx]->setAsCurrent( curwvltnm );
	wvltdrawer_[idx]->display( !idx );
	wvltdrawer_[idx]->funclistselChged.notify( 
				mCB(this,uiSeisWvltMerge,funcSelChg) );
	normalizefld_->attach( alignedAbove, wvltdrawer_[idx] );
	wvltfld_->attach( ensureBelow, wvltdrawer_[idx] );
    }
}


uiSeisWvltMerge::~uiSeisWvltMerge()
{
    for ( int idx=0; idx<wvltdrawer_.size(); idx++ )
	wvltdrawer_[idx]->funclistselChged.remove( 
				mCB(this,uiSeisWvltMerge,funcSelChg) );
    deepErase( wvltset_ );
    deepErase( wvltfuncset_ );
    stackedwvlt_ = 0;
}


#define mGetCurDrawer() uiFuncSelDraw* wd = getCurrentDrawer(); if (!wd) return;
void uiSeisWvltMerge::funcSelChg( CallBacker* )
{
    mGetCurDrawer();
    NotifyStopper nsf( wd->funclistselChged );
    const int selsz = wd->getNrSel();
    if ( selsz==1 && wd->isSelected( wd->getListSize()-1 ) ) 
	return;

    clearStackedWvlt( wd );
    wvltfld_->setSensitive( selsz > 1 );
    if ( selsz <= 1 ) return;

    makeStackedWvlt();
}


void uiSeisWvltMerge::clearStackedWvlt( uiFuncSelDraw* wd )
{
    if ( stackedwvlt_ )
    {
	delete wvltfuncset_.remove( wd->removeLastItem() );
	wvltset_.remove( wvltset_.size()-1 );
	delete stackedwvlt_; stackedwvlt_=0;
    }
}


void uiSeisWvltMerge::makeStackedWvlt()
{
    mGetCurDrawer();
    TypeSet<int> selitems;
    wd->getSelectedItems( selitems );
    const int selsize = selitems.size();

    const char* wvltname = "Stacked Wavelet";
    stackedwvlt_ = new Wavelet( wvltname );
    wvltset_ += stackedwvlt_;
    stackedwvlt_->reSize( maxwvltsize_ );
    for ( int idx=0; idx<maxwvltsize_; idx++ )
	stackedwvlt_->samples()[idx] = 0;

    for ( int selidx=0; selidx<selsize; selidx++ )
    {
	Wavelet* curwvlt = wvltset_[selitems[selidx]];
	stackedwvlt_->set( maxwvltsize_/2, curwvlt->sampleRate() );
	WvltMathFunction* func = wvltfuncset_[selitems[selidx]];
	for ( int idx=0; idx<maxwvltsize_; idx++ )
	{
	    const float coeff = 2*idx-maxwvltsize_+1;
	    const float val = func->getValue( coeff*5*SI().zStep());
	    stackedwvlt_->samples()[idx] += val/selsize; 
	}
    }
    WvltMathFunction* stackedfunc = new WvltMathFunction( stackedwvlt_ );
    wvltfuncset_ += stackedfunc;
    wd->addToList( wvltname );
    wd->addFunction( stackedfunc );
    wd->setAsCurrent( wvltname );
}


void uiSeisWvltMerge::constructDrawer( bool isnormalized )
{
    float minhght=0; float maxhght=0;
    for ( int wvltidx=0; wvltidx<wvltset_.size(); wvltidx++ )
    {
	Wavelet* wvlt = wvltset_[wvltidx];
	if ( isnormalized ) wvlt->normalize();
	const int wvltsz = wvlt->size();
	const float minval = wvlt->getExtrValue(false);
	const float maxval = wvlt->getExtrValue(true);
	if ( wvltsz > maxwvltsize_ ) maxwvltsize_ = wvltsz;
	if ( minval < minhght ) minhght = minval;
	if ( maxval > maxhght ) maxhght = maxval;
    }
    const float stopx = SI().zStep()*maxwvltsize_*5; 
    const float startx = -stopx;
    const StepInterval<float> xaxrg( startx, stopx, 1 );
    const StepInterval<float> yaxrg( minhght, maxhght,
				     (maxhght-minhght)/8);
    uiFuncSelDraw::Setup su; su.name_ = "Wavelet Stacking";
    su.xaxrg_ = xaxrg; su.yaxrg_ = yaxrg;
    wvltdrawer_ += new uiFuncSelDraw( this, su );
}


uiFuncSelDraw* uiSeisWvltMerge::getCurrentDrawer()
{ return wvltdrawer_[normalizefld_->isChecked()]; }


void uiSeisWvltMerge::reloadWvlts()
{
    deepErase( wvltset_ ); deepErase( namelist_ ); stackedwvlt_ = 0;
    IODirEntryList del( IOM().dirPtr(), ctio_.ctxt );
    if ( del.size() < 2 ) { pErrMsg("Not enough wavelets available"); return; }

    for ( int delidx=0; delidx<del.size(); delidx++ )
    {
	const IOObj* ioobj = del[delidx]->ioobj;
	if ( !ioobj ) continue;
	Wavelet* wvlt = Wavelet::get( ioobj );
	wvltset_ += wvlt;
	if ( normalizefld_->isChecked() )
	    wvlt->normalize();
	if ( centerfld_->isChecked() )
	{
	    if ( centerchoicefld_->box()->currentItem() )
		setToMaxEnerPos( wvlt );
	    else
		setToMaxAmplPos( wvlt );
	}
	namelist_.add( ioobj->name() );
    }
}


void uiSeisWvltMerge::reloadFunctions()
{
    for ( int widx=0; widx<wvltdrawer_.size(); widx++ )
    {
	for ( int idx=wvltdrawer_[widx]->getListSize()-1; idx>=0; idx-- )
	    wvltdrawer_[widx]->removeItem( idx );
    }
    deepErase( wvltfuncset_ );
    for ( int widx=0; widx<wvltdrawer_.size(); widx++ )
    {
	for ( int idx=0; idx<wvltset_.size(); idx++ )
	{
	    wvltdrawer_[widx]->addToList( namelist_[idx]->buf() );
	    wvltfuncset_ += new WvltMathFunction( wvltset_[idx] );
	    wvltdrawer_[widx]->addFunction( wvltfuncset_[idx] );
	}
    }
}


void uiSeisWvltMerge::reloadAll( CallBacker* )
{
    NotifyStopper nsf0( wvltdrawer_[0]->funclistselChged );
    NotifyStopper nsf1( wvltdrawer_[1]->funclistselChged );

    reloadWvlts();
    reloadFunctions();

    wvltdrawer_[normalizefld_->isChecked()]->setAsCurrent( curwvltnm_ );
    funcSelChg( 0 );
    wvltdrawer_[0]->display( !normalizefld_->isChecked() );
    wvltdrawer_[1]->display( normalizefld_->isChecked() );
}


void uiSeisWvltMerge::setToMaxEnerPos( Wavelet* wvlt )
{
    WaveletAttrib wvltattr( wvlt );
    Array1DImpl<float> hilb ( wvlt->size() );
    wvltattr.getHilbert( hilb );

    float max = 0; 	int centeridx = 0;
    for ( int idx=0; idx<wvlt->size(); idx++ )
    {
	float val = fabs( hilb.get(idx) );
	if ( max < val )
	{
	    max = val; 
	    centeridx = idx;
	}
    }
    wvlt->set( centeridx, wvlt->sampleRate() );
}


void uiSeisWvltMerge::setToMaxAmplPos( Wavelet* wvlt )
{
    int centeridx = 0;
    float extrval = wvlt->getExtrValue( true );
    const float minval = wvlt->getExtrValue( false );
    if ( fabs(minval) > fabs(extrval) )
	extrval = minval;

    for ( int idx=0; idx<wvlt->size(); idx++ )
    {
	if ( mIsEqual(wvlt->samples()[idx],extrval,mDefEps) )
	{
	    centeridx = idx;
	    break;
	}
    }
    wvlt->set( centeridx, wvlt->sampleRate() );
}


void uiSeisWvltMerge::centerChged( CallBacker* )
{
    centerchoicefld_->display( centerfld_->isChecked() );
}


bool uiSeisWvltMerge::acceptOK( CallBacker* )
{
    if ( !stackedwvlt_ )
	mErrRet( "there is no stacked wavelet to be saved" );
    return putWvlt( *stackedwvlt_ );
}


uiSeisWvltMerge::WvltMathFunction::WvltMathFunction( const Wavelet* wvlt )
    		: samples_(wvlt->samples())
		, samppos_(wvlt->samplePositions())  
		, size_(wvlt->size())				       
		{}  

float uiSeisWvltMerge::WvltMathFunction::getValue( float t ) const
{
    float x = ( t*0.1 - samppos_.start );
    x /= samppos_.step;
    const int x1 = int(x);
    if ( x1 > size_-1 || x1<0 )
	return 0;
    else if ( x1 == size_-1 )
	return samples_[x1];
    const float val1 = samples_[x1];
    const int x2 = x1+1 ;
    const float val2 = samples_[x2];
    const float factor = ( x-x1 )/( x2-x1 ); 

    if ( x1==0 || x2 == size_-1 )
	return Interpolate::linearReg1D( val1, val2, factor );
    else
    {
	float val0 = samples_[x1-1]; 
	float val3 = samples_[x2+1];
	Interpolate::PolyReg1D<float> pr;
	pr.set( val0, val1, val2, val3 );
	return pr.apply( factor );
    }
}

