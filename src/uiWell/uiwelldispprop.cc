/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Dec 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldispprop.cc,v 1.9 2008-12-22 15:50:49 cvsbruno Exp $";

#include "uiwelldispprop.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uispinbox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uicoltabman.h"
#include "uicoltabtools.h"

#include "welldata.h"
#include "multiid.h"
#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"
#include "multiid.h"


uiWellDispProperties::uiWellDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::BasicProps& pr )
    : uiGroup(p,"Well display properties group")
    , props_(pr)
    , propChanged(this)

{
    szfld_ = new uiSpinBox( this, 0, "Size" );
    szfld_->setInterval( StepInterval<int>(1,mUdf(int),1) );
    szfld_->setValue(  props_.size_ );
    szfld_->valueChanging.notify( mCB(this,uiWellDispProperties,propChg) );
    new uiLabel( this, su.mysztxt_ , szfld_ );

    uiColorInput::Setup csu( props_.color_ );
    csu.lbltxt( su.mycoltxt_ ).withalpha( false );
    BufferString dlgtxt( "Select " );
    dlgtxt += su.mycoltxt_; dlgtxt += " for "; dlgtxt += props_.subjectName();
    colfld_ = new uiColorInput( this, csu, su.mycoltxt_ );
    colfld_->attach( alignedBelow, szfld_ );
    colfld_->colorchanged.notify( mCB(this,uiWellDispProperties,propChg) );

    setHAlignObj( colfld_ );
    
}

void uiWellDispProperties::propChg( CallBacker* )
{
    getFromScreen();
    propChanged.trigger();
}


void uiWellDispProperties::putToScreen()
{
    szfld_->setValue( props_.size_ );
    colfld_->setColor( props_.color_ );
    doPutToScreen();
}


void uiWellDispProperties::getFromScreen()
{
    props_.size_ = szfld_->getValue();
    props_.color_ = colfld_->color();
    doGetFromScreen();
}


uiWellTrackDispProperties::uiWellTrackDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Track& tp )
    : uiWellDispProperties(p,su,tp)
{
    dispabovefld_ = new uiCheckBox( this, "Above" );
    dispabovefld_->attach( alignedBelow, colfld_ );
    dispabovefld_->activated.notify( mCB(this,uiWellTrackDispProperties,propChg) );
    dispbelowfld_ = new uiCheckBox( this, "Below" );
    dispbelowfld_->attach( rightOf, dispabovefld_ );
    dispbelowfld_->activated.notify( mCB(this,uiWellTrackDispProperties,propChg) );
    uiLabel* lbl = new uiLabel( this, "Display well name" , dispabovefld_ );
    lbl = new uiLabel( this, "track" );
    lbl->attach( rightOf, dispbelowfld_ );
}


void uiWellTrackDispProperties::doPutToScreen()
{
    dispabovefld_->setChecked( trackprops().dispabove_ );
    dispbelowfld_->setChecked( trackprops().dispbelow_ );
}


void uiWellTrackDispProperties::doGetFromScreen()
{
    trackprops().dispabove_ = dispabovefld_->isChecked();
    trackprops().dispbelow_ = dispbelowfld_->isChecked();
}


uiWellMarkersDispProperties::uiWellMarkersDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Markers& mp )
    : uiWellDispProperties(p,su,mp)
{
    circfld_ = new uiGenInput( this, "Shape",
			       BoolInpSpec(true,"Circular","Square") );
    circfld_->attach( alignedBelow, colfld_ );
    circfld_->valuechanged.notify( mCB(this,uiWellMarkersDispProperties,propChg) );
}


void uiWellMarkersDispProperties::doPutToScreen()
{
    circfld_->setValue( mrkprops().circular_ );
}


void uiWellMarkersDispProperties::doGetFromScreen()
{
    mrkprops().circular_ = circfld_->getBoolValue();
}


uiWellLogDispProperties::uiWellLogDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Log& lp, 
				Well::LogSet* wl)
    : uiWellDispProperties(p,su,lp)
{
    wl_ = wl;
    stylefld_ = new uiGenInput( this, "Style", 
			       BoolInpSpec( true,"Well log","Seismic" ) );
    stylefld_->attach( alignedAbove, szfld_ );
    stylefld_->valuechanged.notify( mCB(this,uiWellLogDispProperties,
	                                     isSeismicSel) );
    stylefld_->valuechanged.notify( mCB(this,uiWellLogDispProperties,propChg) );

    rangefld_ = new uiGenInput( this, "data range",
			     FloatInpIntervalSpec()
			     .setName(BufferString(" range start"),0)
			     .setName(BufferString(" range stop"),1) );
    rangefld_->attach( alignedAbove, stylefld_ );
    rangefld_->valuechanged.notify( mCB(this,uiWellLogDispProperties,
	       						choiceSel) );
    rangefld_->valuechanged.notify( mCB(this,uiWellLogDispProperties,propChg) );
   
    const char* choice[] = { "clip rate", "data range", 0 };
    cliprangefld_ = new uiGenInput( this, "Specify",
				StringListInpSpec(choice) );
    cliprangefld_->attach( alignedAbove, rangefld_ );
    cliprangefld_->valuechanged.notify( mCB(this,uiWellLogDispProperties,
							choiceSel));
   
    clipratefld_ = new uiGenInput( this, "Clip rate", StringInpSpec() );
    clipratefld_->setElemSzPol( uiObject::Small );
    clipratefld_->attach( alignedBelow, cliprangefld_ );
    clipratefld_->valuechanged.notify( mCB(this,uiWellLogDispProperties,
	       						choiceSel) );
    clipratefld_->valuechanged.notify( mCB(this,uiWellLogDispProperties,
							   propChg) );

    logfillfld_ = new uiCheckBox( this, "log filled" );
    logfillfld_->attach( rightOf, szfld_ );   
    logfillfld_->activated.notify( mCB(this,uiWellLogDispProperties,
    							isFilledSel));
    logfillfld_->activated.notify( mCB(this,uiWellLogDispProperties,
							   propChg) );

    singlfillcolfld_ = new uiCheckBox( this, "single color" );
    singlfillcolfld_->setName( BufferString("single color") );
    singlfillcolfld_->attach(rightOf, logfillfld_);
    singlfillcolfld_->activated.notify( mCB(this,uiWellLogDispProperties,
    							isFilledSel));
    singlfillcolfld_->activated.notify( mCB(this,uiWellLogDispProperties,
							   propChg) );

    coltablistfld_ = new uiComboBox( this, "Table selection" );
    coltablistfld_->attach( alignedBelow , logfillfld_ );
    coltablistfld_->display( false );
    BufferStringSet allctnms;
    ColTab::SM().getSequenceNames( allctnms );
    allctnms.sort();
    for ( int idx=0; idx<allctnms.size(); idx++ )
    {
	const int seqidx = ColTab::SM().indexOf( allctnms.get(idx) );
	if ( seqidx<0 ) continue;
	const ColTab::Sequence* seq = ColTab::SM().get( seqidx );
	coltablistfld_->addItem( seq->name() );
	coltablistfld_->setPixmap( ioPixmap(*seq,16,10), idx );
    }
    coltablistfld_->selectionChanged.notify( mCB(this,uiWellLogDispProperties,
								propChg) );

    BufferStringSet lognames;
    lognames.setIsOwner(false);
    for ( int idx=0; idx< wl_->size(); idx++ )
    lognames.addIfNew( wl_->getLog(idx).name() );
    lognames.sort();
    BufferString sellbl( "Select log" );
    logsfld_ = new uiLabeledComboBox( this, sellbl );
    logsfld_->box()->addItem("None");
    logsfld_->box()->addItems( lognames );
    logsfld_->attach( alignedAbove, cliprangefld_ );
    logsfld_->box()->selectionChanged.notify( mCB(this, uiWellLogDispProperties,
				 		logSel));
    logsfld_->box()->selectionChanged.notify( mCB(this, uiWellLogDispProperties,
				 		updateRange));
    logsfld_->box()->selectionChanged.notify( mCB(this,
	       			uiWellLogDispProperties, updateFillRange));


    BufferString selfilllbl( "Select log for filling color scale" );
    filllogsfld_ = new uiLabeledComboBox( this, selfilllbl );
  //  filllogsfld_->box()->addItem("None");
    filllogsfld_->box()->addItems( lognames );
    filllogsfld_->attach( alignedBelow, coltablistfld_ );
    filllogsfld_->box()->selectionChanged.notify( mCB(this, 
				uiWellLogDispProperties,logSel));
    filllogsfld_->box()->selectionChanged.notify( mCB(this,
	       			uiWellLogDispProperties, updateFillRange));

    repeatfld_ = new uiGenInput( this, "logs number", FloatInpSpec() );
    repeatfld_->setElemSzPol( uiObject::Small );
    repeatfld_->valuechanged.notify( mCB(this,uiWellLogDispProperties,
							isRepeatSel) );
    repeatfld_->attach(alignedBelow, colfld_);
    repeatfld_->valuechanged.notify( mCB(this,uiWellLogDispProperties,propChg));

    ovlapfld_ = new uiGenInput( this, "Overlapping",
						FloatInpSpec() );
    ovlapfld_->setElemSzPol( uiObject::Small );
    ovlapfld_->attach( rightOf, repeatfld_ );
    ovlapfld_->valuechanged.notify( mCB(this,uiWellLogDispProperties,propChg) );

    seiscolorfld_ = new uiColorInput( this,
		                 uiColorInput::Setup(logprops().seiscolor_)
			        .lbltxt("Filling Color") );
    seiscolorfld_->attach( rightOf, colfld_);
    seiscolorfld_->display(false);
    seiscolorfld_->colorchanged.notify( mCB(this,uiWellLogDispProperties,
								propChg) );
    recoverProp();
}


void uiWellLogDispProperties::doPutToScreen()
{
    logsfld_-> box() -> setText( logprops().name_ );
    stylefld_->setValue( logprops().iswelllog_ ); 
    rangefld_->setValue( logprops().range_ ); 
    coltablistfld_->setText( logprops().seqname_ ); 
    logfillfld_->setChecked( logprops().islogfill_ );
    clipratefld_->setValue( logprops().cliprate_ );
    cliprangefld_->setValue( logprops().isdatarange_ );
    if ( mIsUdf( logprops().cliprate_ ) )
    {
	cliprangefld_->setValue( true );
	clipratefld_->setValue( 0.0 );
    }
    repeatfld_->setValue( logprops().repeat_ );
    ovlapfld_->setValue( logprops().repeatovlap_);
    seiscolorfld_->setColor( logprops().seiscolor_);
    filllogsfld_-> box() -> setText( logprops().fillname_ );
    singlfillcolfld_ -> setChecked(logprops().issinglecol_); 
}


void uiWellLogDispProperties::doGetFromScreen()
{
    logprops().iswelllog_ = stylefld_->getBoolValue();
    logprops().cliprate_ = clipratefld_->getfValue();
    logprops().range_ = rangefld_->getFInterval();
    logprops().isdatarange_ = cliprangefld_->getBoolValue();
    logprops().islogfill_ = logfillfld_->isChecked();
    logprops().seqname_ = coltablistfld_-> text();
    if ( stylefld_->getBoolValue() == true )
	logprops().repeat_ = 1;
    else
	logprops().repeat_ = repeatfld_->getIntValue();
    logprops().repeatovlap_ = ovlapfld_->getfValue();
    logprops().seiscolor_ = seiscolorfld_->color();
    logprops().name_ = logsfld_->box()->text();
    logprops().fillname_ = filllogsfld_->box()->text();
    logprops().issinglecol_ = singlfillcolfld_->isChecked();
}


void uiWellLogDispProperties::isFilledSel( CallBacker* )
{
    const bool iswelllog = stylefld_->getBoolValue();
    const bool isfilled = logfillfld_->isChecked();
    const bool issinglecol = singlfillcolfld_->isChecked();
    singlfillcolfld_->display( isfilled && iswelllog);
    coltablistfld_->display( iswelllog &&  isfilled && !issinglecol );
    seiscolorfld_->display(  !iswelllog || issinglecol && isfilled );
    filllogsfld_->display( iswelllog &&  isfilled && !issinglecol );
}


void uiWellLogDispProperties::isRepeatSel( CallBacker* )
{
    const bool isrepeat = ( repeatfld_-> isChecked()
	    				 && repeatfld_->getIntValue() > 0 );
    const bool iswelllog = stylefld_->getBoolValue();
    if (iswelllog)
    {
	repeatfld_-> setValue( 1 );
        ovlapfld_-> setValue( 50 );
    }
}


void uiWellLogDispProperties::isSeismicSel( CallBacker* )
{
    const bool iswelllog = stylefld_->getBoolValue();
    repeatfld_->display( !iswelllog );
    ovlapfld_->display( !iswelllog );
    logfillfld_->display( iswelllog );
    if (iswelllog)
	repeatfld_->setValue(1);
    BufferString str = "Select ";
    str += "filling color";
    isFilledSel(0);
}


void uiWellLogDispProperties::recoverProp( )
{
    doPutToScreen();
    if ( logprops().name_ == "None" || logprops().name_ ==  "none" ) selNone();
    isSeismicSel(0);
    choiceSel(0);
    isFilledSel(0);
}

void uiWellLogDispProperties::setRangeFields( Interval<float>& range )
{
    rangefld_->setValue( range );
    valuerange_ = range;
}


void uiWellLogDispProperties::logSel( CallBacker* )
{
    setFieldVals( false );
    BufferString fillname = filllogsfld_->box()->text();
    if ( mIsUdf(fillvaluerange_.start) || mIsUdf(fillvaluerange_.stop) ||
	    fillvaluerange_.start == 0 && fillvaluerange_.stop == 0 )
    {
	filllogsfld_-> box() -> setText( logsfld_->box() -> text() );
    }
}


void uiWellLogDispProperties::selNone()
{
    rangefld_->setValue( Interval<float>(0,0) );
    colfld_->setColor( Color::White );
    seiscolorfld_->setColor( Color::White );
    stylefld_->setValue( true );
    setFldSensitive( false );
    cliprangefld_->setValue( true );
    clipratefld_->setValue( 0.0 );
    repeatfld_->setValue( 0 );
    ovlapfld_->setValue( 0 );
    logfillfld_->setChecked( false );
    singlfillcolfld_->setChecked( false );
    //filllogsfld_-> box() -> setText( "None" );
    //logscfld_->setChecked( false );
}



void uiWellLogDispProperties::setFldSensitive( bool yn )
{
    rangefld_->setSensitive( yn );
    cliprangefld_->setSensitive( yn );
    colfld_->setSensitive( yn );
    seiscolorfld_->setSensitive( yn );
    stylefld_->setSensitive( yn );
    clipratefld_->setSensitive( yn );
    repeatfld_->setSensitive( yn );
    logfillfld_->setSensitive( yn );
    coltablistfld_->setSensitive( yn );
    logfillfld_->setSensitive( yn );
    szfld_->setSensitive( yn );
    singlfillcolfld_->setSensitive( yn );
}


void uiWellLogDispProperties::choiceSel( CallBacker* )
{
    const int isdatarange = cliprangefld_->getBoolValue();
    rangefld_->display( isdatarange );
    clipratefld_->display( !isdatarange );
}


void uiWellLogDispProperties::setFieldVals( bool def )
{
    BufferString sel = logsfld_->box()->text();
    if ( sel == "None")
    {
	selNone();
	return;
    }

    setFldSensitive( true );
}



void uiWellLogDispProperties::updateRange( CallBacker* )
{
	const char* lognm = logsfld_->box()->textOfItem(
			    logsfld_->box()->currentItem() );;
	const int logno = wl_->indexOf( lognm );
	if ( logno < 0 )
	return;

	Interval<float> range; 
	range = wl_->getLog(logno).valueRange();
	setRangeFields( range );
	propChanged.trigger();
	return;

    calcLogValueRange();

    if ( mIsUdf(valuerange_.start) || mIsUdf(valuerange_.stop) )
	valuerange_.set(0,0);
    else
	setRangeFields( valuerange_ );

    propChanged.trigger();
}


void uiWellLogDispProperties::updateFillRange( CallBacker* )
{
	const char* lognm = filllogsfld_->box()->textOfItem(
			    filllogsfld_->box()->currentItem() );;
	const int logno = wl_->indexOf( lognm );
	if ( logno < 0 )
	return;

    calcFillLogValueRange();

    if ( mIsUdf(fillvaluerange_.start) || mIsUdf(fillvaluerange_.stop) )
	fillvaluerange_.set(0,0);
    else 
	 logprops().fillrange_ = fillvaluerange_;

    propChanged.trigger();
}


void uiWellLogDispProperties::calcLogValueRange( )
{
    valuerange_.set( mUdf(float), -mUdf(float) );
    const char* lognm =  logsfld_->box()->textOfItem(
	                    logsfld_->box()->currentItem() ); 
    for ( int idy=0; idy<wl_->size(); idy++ )
    {
	if ( !strcmp(lognm, wl_->getLog(idy).name()) )
	{
	    const int logno = wl_->indexOf( lognm );
	    Interval<float> range = wl_->getLog(logno).valueRange();
	    if ( valuerange_.start > range.start )
	    valuerange_.start = range.start;
	    if ( valuerange_.stop < range.stop )
	    valuerange_.stop = range.stop;
	}
    }
}


void uiWellLogDispProperties::calcFillLogValueRange( )
{
    fillvaluerange_.set( mUdf(float), -mUdf(float) );
    const char* lognm =  filllogsfld_->box()->textOfItem(
	                    filllogsfld_->box()->currentItem() ); 
    for ( int idy=0; idy<wl_->size(); idy++ )
    {
	if ( !strcmp(lognm,wl_->getLog(idy).name()) )
	{
	    const int logno = wl_->indexOf( lognm );
	    Interval<float> range = wl_->getLog(logno).valueRange();
	    if ( fillvaluerange_.start > range.start )
		fillvaluerange_.start = range.start;
	    if ( fillvaluerange_.stop < range.stop )
		fillvaluerange_.stop = range.stop;
	}
    }
}

/*
void uiWellLogDispProperties::calcRange( const char* lognm,
       					        	Interval<float> valr )
{
    valr.set( mUdf(float), -mUdf(float) );
    for ( int idy=0; idy<wl_->size(); idy++ )
    {
	if ( !strcmp(lognm,wl_->getLog(idy).name()) )
	{
	    const int logno = wl_->indexOf( lognm );
	    Interval<float> range = wl_->getLog(logno).valueRange();
	    if ( valr.start > range.start )
		valr.start = range.start;
	    if ( valr.stop < range.stop )
		valr.stop = range.stop;
	}
    }
}
*/
