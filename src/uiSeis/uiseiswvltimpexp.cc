/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiseiswvltimpexp.h"
#include "wavelet.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "oddirs.h"
#include "tabledef.h"
#include "od_iostream.h"
#include "survinfo.h"

#include "uiioobjsel.h"
#include "uitblimpexpdatasel.h"
#include "uifileinput.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "od_helpids.h"

#include <math.h>


uiSeisWvltImp::uiSeisWvltImp( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Wavelet",mNoDlgTitle,
                                 mODHelpKey(mSeisWvltImpHelpID) ))
    , fd_(*WaveletAscIO::getDesc())
    , ctio_(*mMkCtxtIOObj(Wavelet))
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiFileInput( this, "Input ASCII file", uiFileInput::Setup()
		      .withexamine(true).examstyle(File::Table) );
    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, inpfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, 
                  mODHelpKey(mSeisWvltImpParsHelpID)  );
    dataselfld_->attach( alignedBelow, inpfld_ );
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, dataselfld_ );

    scalefld_ = new uiGenInput( this, "Scale factor for samples",
				FloatInpSpec(1) );
    scalefld_->attach( alignedBelow, dataselfld_ );
    scalefld_->attach( ensureBelow, sep );

    ctio_.ctxt.forread = false;
    wvltfld_ = new uiIOObjSel( this, ctio_ );
    wvltfld_->attach( alignedBelow, scalefld_ );
}


uiSeisWvltImp::~uiSeisWvltImp()
{
    delete ctio_.ioobj; delete &ctio_;
    delete &fd_;
}

#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }



bool uiSeisWvltImp::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( "Please enter the input file name" )
    if ( !wvltfld_->commitInput() )
	mErrRet( !wvltfld_->isEmpty() ? 0
		: "Please enter a name for the new wavelet" )
    if ( !dataselfld_->commit() )
	return false;
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( "Cannot open input file" )

    WaveletAscIO aio( fd_ );
    PtrMan<Wavelet> wvlt = aio.get( strm );
    if ( !wvlt )
	mErrRet(aio.errMsg())

    const int nrsamps = wvlt->size();
    int maxsamp = 0;
    float maxval = fabs( wvlt->samples()[maxsamp] );
    for ( int idx=1; idx<nrsamps; idx++ )
    {
	const float val = fabs( wvlt->samples()[idx] );
	if ( val > maxval )
	    { maxval = val; maxsamp = idx; }
    }
    if ( maxsamp != wvlt->centerSample() )
    {
	BufferString msg( "Highest amplitude is at sample: " );
	msg += maxsamp + 1;
	msg += "\nThe center sample found is: ";
	msg += wvlt->centerSample() + 1;
	msg += "\n\nDo you want to reposition the center sample,"
	       "\nSo it will be at the highest amplitude position?";
	if ( uiMSG().askGoOn( msg ) )
	    wvlt->setCenterSample( maxsamp );
    }

    const float fac = scalefld_->getfValue();
    if ( fac != 0 && !mIsUdf(fac) && fac != 1 )
    {
	float* samps = wvlt->samples();
	for ( int idx=0; idx<wvlt->size(); idx++ )
	    samps[idx] *= fac;
    }

    if ( !wvlt->put(ctio_.ioobj) )
	mErrRet( "Cannot store wavelet on disk" )

    return true;
}


MultiID uiSeisWvltImp::selKey() const
{
    return ctio_.ioobj ? ctio_.ioobj->key() : MultiID("");
}


// uiSeisWvltExp
uiSeisWvltExp::uiSeisWvltExp( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Export Wavelet",mNoDlgTitle,
                                 mODHelpKey(mSeisWvltImpHelpID) ))
{
    setOkText( uiStrings::sExport() );

    wvltfld_ = new uiIOObjSel( this, mIOObjContext(Wavelet) );

    addzfld_ = new uiGenInput( this, BufferString("Output ",
				     SI().zIsTime()?"time":"depth"),
				     BoolInpSpec(true) );
    addzfld_->attach( alignedBelow, wvltfld_ );

    outpfld_ = new uiFileInput( this, "Output ASCII file", uiFileInput::Setup()
				.forread(false) );
    outpfld_->attach( alignedBelow, addzfld_ );
}


bool uiSeisWvltExp::acceptOK( CallBacker* )
{
    const IOObj* ioobj = wvltfld_->ioobj();
    if ( !ioobj ) return false;
    const BufferString fnm( outpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( "Please enter the output file name" )

    PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
    if ( !wvlt )
	mErrRet( "Cannot read wavelet" )
    if ( wvlt->size() < 1 )
	mErrRet( "Empty wavelet" )
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( "Cannot open output file" )

    const bool addz = addzfld_->getBoolValue();
    const float zfac = mCast( float, SI().zDomain().userFactor() );
    const StepInterval<float> zpos( wvlt->samplePositions() );
    for ( int idx=0; idx<wvlt->size(); idx++ )
    {
	if ( addz )
	{
	    const float zval = zfac * zpos.atIndex(idx);
	    const od_int64 izval = mRounded( od_int64, 1000 * zval );
	    strm << toString( izval * 0.001 ) << '\t';
	}
	strm << toString(wvlt->samples()[idx]) << '\n';
    }

    if ( !strm.isOK() )
	mErrRet( "Possible error during write" );

    return true;
}
