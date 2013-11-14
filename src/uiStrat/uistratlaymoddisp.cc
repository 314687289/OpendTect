/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsimplelaymoddisp.h"
#include "uistratlaymodtools.h"
#include "uistrateditlayer.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uigeninput.h"
#include "uifiledlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiaxishandler.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratreftree.h"
#include "od_iostream.h"
#include "survinfo.h"
#include "property.h"
#include "keystrs.h"
#include "envvars.h"
#include "oddirs.h"

#define mGetConvZ(var,conv) \
    if ( SI().depthsInFeet() ) var *= conv
#define mGetRealZ(var) mGetConvZ(var,mFromFeetFactorF)
#define mGetDispZ(var) mGetConvZ(var,mToFeetFactorF)
#define mGetDispZrg(src,target) \
    Interval<float> target( src ); \
    if ( SI().depthsInFeet() ) \
	target.scale( mToFeetFactorF )

static const int cMaxNrLayers4RectDisp = 50000; // Simple displayer


uiStratLayerModelDisp::uiStratLayerModelDisp( uiStratLayModEditTools& t,
					  const Strat::LayerModelProvider& lmp )
    : uiGroup(t.parent(),"LayerModel display")
    , tools_(t)
    , lmp_(lmp)
    , zrg_(0,1)
    , selseqidx_(-1)
    , flattened_(false)
    , fluidreplon_(false)
    , frtxtitm_(0)
    , isbrinefilled_(true)
    , sequenceSelected(this)
    , genNewModelNeeded(this)
    , rangeChanged(this)   
    , modelEdited(this)   
    , infoChanged(this)   
    , dispPropChanged(this)
{
}


uiStratLayerModelDisp::~uiStratLayerModelDisp()
{
}


const Strat::LayerModel& uiStratLayerModelDisp::layerModel() const
{
    return lmp_.getCurrent();
}


void uiStratLayerModelDisp::selectSequence( int selidx )
{
    selseqidx_ = selidx;
    drawSelectedSequence();
}


void uiStratLayerModelDisp::setFlattened( bool yn )
{
    const bool domodelchg = flattened_ || yn;
    flattened_ = yn;
    if ( domodelchg )
	modelChanged();
    else
	doLevelChg();
}


bool uiStratLayerModelDisp::haveAnyZoom() const
{
    const int nrseqs = layerModel().size();
    mGetDispZrg(zrg_,dispzrg);
    uiWorldRect wr( 1, dispzrg.start, nrseqs, dispzrg.stop );
    return zoomBox().isInside( wr, 1e-5 );
}


float uiStratLayerModelDisp::getLayerPropValue( const Strat::Layer& lay,
						const PropertyRef* pr,
       						int propidx ) const
{
    return propidx < lay.nrValues() ? lay.value( propidx ) : mUdf(float);
}


void uiStratLayerModelDisp::displayFRText()
{
    if ( !frtxtitm_ )
	frtxtitm_ = scene().addItem( new uiTextItem( "<---empty--->",
				 mAlignment(HCenter,VCenter) ) );
    frtxtitm_->setText( isbrinefilled_ ? "Brine filled" : "Hydrocarbon filled");
    frtxtitm_->setPenColor( Color::Black() );
    const int xpos = mNINT32( scene().width()/2 );
    const int ypos = mNINT32( scene().height()-10 );
    frtxtitm_->setPos( uiPoint(xpos,ypos) );
    frtxtitm_->setZValue( 999999 );
    frtxtitm_->setVisible( fluidreplon_ );
}


#define mErrRet(s) \
{ \
    uiMainWin* mw = uiMSG().setMainWin( parent()->mainwin() ); \
    uiMSG().error(s); \
    uiMSG().setMainWin(mw); \
    return false; \
}


bool uiStratLayerModelDisp::doLayerModelIO( bool foradd )
{
    const Strat::LayerModel& lm = layerModel();
    if ( !foradd && lm.isEmpty() )
	mErrRet( "Please generate some layer sequences" )

    mDefineStaticLocalObject( BufferString, fixeddumpfnm, 
			      = GetEnvVar("OD_FIXED_LAYMOD_DUMPFILE") );
    BufferString dumpfnm( fixeddumpfnm );
    if ( dumpfnm.isEmpty() )
    {
	uiFileDialog dlg( this, foradd, 0, 0, "Select layer model dump file" );
	dlg.setDirectory( GetDataDir() );
	if ( !dlg.go() ) return false;
	dumpfnm = dlg.fileName();
    }

    if ( !foradd )
    {
	od_ostream strm( dumpfnm );
	if ( !strm.isOK() )
	    mErrRet( BufferString("Cannot open:\n",dumpfnm,"\nfor write") )
	if ( !lm.write(strm) )
	    mErrRet( "Unknown error during write ..." )
	return false;
    }

    od_istream strm( dumpfnm );
    if ( !strm.isOK() )
	mErrRet( BufferString("Cannot open:\n",dumpfnm,"\nfor read") )

    Strat::LayerModel newlm;
    if ( !newlm.read(strm) )
	mErrRet( "Cannot read layer model from file."
		 "\nFile may not be a layer model file" )

    for ( int ils=0; ils<newlm.size(); ils++ )
	const_cast<Strat::LayerModel&>(lm)
			    .addSequence( newlm.sequence( ils ) );

    return true;
}


bool uiStratLayerModelDisp::getCurPropDispPars(
	LMPropSpecificDispPars& pars ) const
{
    LMPropSpecificDispPars disppars;
    disppars.propnm_ = tools_.selProp();
    const int curpropidx = lmdisppars_.indexOf( disppars );
    if ( curpropidx<0 )
	return false;
    pars = lmdisppars_[curpropidx];
    return true;
}


bool uiStratLayerModelDisp::setPropDispPars( const LMPropSpecificDispPars& pars)
{
    BufferStringSet propnms;
    for ( int idx=0; idx<layerModel().propertyRefs().size(); idx++ )
	propnms.add( layerModel().propertyRefs()[idx]->name() );

    if ( !propnms.isPresent(pars.propnm_) )
	return false;
    const int propidx = lmdisppars_.indexOf( pars );
    if ( propidx<0 )
	lmdisppars_ += pars;
    else
	lmdisppars_[propidx] = pars;
    return true;
}


//=========================================================================>>


uiStratSimpleLayerModelDisp::uiStratSimpleLayerModelDisp(
		uiStratLayModEditTools& t, const Strat::LayerModelProvider& l )
    : uiStratLayerModelDisp(t,l)
    , emptyitm_(0)
    , zoomboxitm_(0)
    , dispprop_(1)
    , dispeach_(1)
    , zoomwr_(mUdf(double),0,0,0)
    , fillmdls_(false)
    , uselithcols_(true)
    , showzoomed_(true)
    , vrg_(0,1)
    , logblcklineitms_(*new uiGraphicsItemSet)
    , logblckrectitms_(*new uiGraphicsItemSet)
    , lvlitms_(*new uiGraphicsItemSet)
    , contitms_(*new uiGraphicsItemSet)
    , selseqitm_(0)
    , selectedlevel_(-1)
    , selectedcontent_(0)
    , allcontents_(false)
{
    gv_ = new uiGraphicsView( this, "LayerModel display" );
    gv_->disableScrollZoom();
    gv_->setPrefWidth( 800 ); gv_->setPrefHeight( 300 );
    gv_->getMouseEventHandler().buttonReleased.notify(
			mCB(this,uiStratSimpleLayerModelDisp,usrClicked) );
    gv_->getMouseEventHandler().doubleClick.notify(
			mCB(this,uiStratSimpleLayerModelDisp,doubleClicked) );
    gv_->getMouseEventHandler().movement.notify(
			mCB(this,uiStratSimpleLayerModelDisp,mouseMoved) );

    const uiBorder border( 10 );
    uiAxisHandler::Setup xahsu( uiRect::Top );
    xahsu.border( border ).nogridline( true );
    xax_ = new uiAxisHandler( &scene(), xahsu );
    uiAxisHandler::Setup yahsu( uiRect::Left );
    const BufferString zlbl( "Depth (", SI().depthsInFeet() ? "ft" : "m", ")" );
    yahsu.border( border ).name( zlbl );
    yax_ = new uiAxisHandler( &scene(), yahsu );
    yax_->setEnd( xax_ );
    xax_->setBegin( yax_ );

    const CallBack redrawcb( mCB(this,uiStratSimpleLayerModelDisp,reDrawCB) );
    gv_->reSize.notify( redrawcb );
    gv_->reDrawNeeded.notify( redrawcb );
    tools_.selPropChg.notify( redrawcb );
    tools_.selLevelChg.notify( redrawcb );
    tools_.selContentChg.notify( redrawcb );
    tools_.dispEachChg.notify( redrawcb );
    tools_.dispZoomedChg.notify( redrawcb );
    tools_.dispLithChg.notify( redrawcb );
}


uiStratSimpleLayerModelDisp::~uiStratSimpleLayerModelDisp()
{
    eraseAll();
    delete &lvlitms_;
    delete &logblcklineitms_;
    delete &logblckrectitms_;
}


void uiStratSimpleLayerModelDisp::eraseAll()
{
    logblcklineitms_.erase();
    logblckrectitms_.erase();
    lvlitms_.erase();
    lvldpths_.erase();
    delete selseqitm_; selseqitm_ = 0;
    delete emptyitm_; emptyitm_ = 0;
    delete frtxtitm_; frtxtitm_ = 0;
}


uiGraphicsScene& uiStratSimpleLayerModelDisp::scene() const
{
    return const_cast<uiStratSimpleLayerModelDisp*>(this)->gv_->scene();
}


int uiStratSimpleLayerModelDisp::getClickedModelNr() const
{
    MouseEventHandler& mevh = gv_->getMouseEventHandler();
    if ( layerModel().isEmpty() || !mevh.hasEvent() || mevh.isHandled() )
	return -1;
    const MouseEvent& mev = mevh.event();
    const float xsel = xax_->getVal( mev.pos().x );
    int selidx = mNINT32( xsel ) - 1;
    if ( selidx < 0 || selidx >= layerModel().size() )
	selidx = -1;
    return selidx;
}


void uiStratSimpleLayerModelDisp::mouseMoved( CallBacker* )
{
    IOPar statusbarmsg;
    statusbarmsg.set( "Model Number", getClickedModelNr() );
    const MouseEvent& mev = gv_->getMouseEventHandler().event();
    float depth = yax_->getVal( mev.pos().y );
    if ( !Math::IsNormalNumber(depth) )
    {
	mDefineStaticLocalObject( bool, havewarned, = false );
	if ( !havewarned )
	    { havewarned = true; pErrMsg("Invalid number from axis handler"); }
	depth = 0;
    }
    statusbarmsg.set( "Depth", depth );
    infoChanged.trigger( statusbarmsg, this );
}


void uiStratSimpleLayerModelDisp::usrClicked( CallBacker* )
{
    const int selidx = getClickedModelNr();
    if ( selidx < 0 ) return;

    MouseEventHandler& mevh = gv_->getMouseEventHandler();
    if ( OD::rightMouseButton(mevh.event().buttonState() ) )
	handleRightClick(selidx);
    else
    {
	selectSequence( selidx );
	sequenceSelected.trigger();
	mevh.setHandled( true );
    }
}


void uiStratSimpleLayerModelDisp::handleRightClick( int selidx )
{
    if ( selidx < 0 || selidx >= layerModel().size() )
	return;

    Strat::LayerSequence& ls = const_cast<Strat::LayerSequence&>(
	    				layerModel().sequence( selidx ) );
    ObjectSet<Strat::Layer>& lays = ls.layers();
    MouseEventHandler& mevh = gv_->getMouseEventHandler();
    float zsel = yax_->getVal( mevh.event().pos().y );
    mGetRealZ( zsel );
    mevh.setHandled( true );
    if ( flattened_ )
    {
	const float lvlz = lvldpths_[selidx];
	if ( mIsUdf(lvlz) )
	    return;
	zsel += lvlz;
    }

    const int layidx = ls.layerIdxAtZ( zsel );
    if ( lays.isEmpty() || layidx < 0 )
	return;

    uiMenu mnu( parent(), "Action" );
    mnu.insertItem( new uiAction("&Properties ..."), 0 );
    mnu.insertItem( new uiAction("&Remove layer ..."), 1 );
    mnu.insertItem( new uiAction("Remove this &Well"), 2 );
    mnu.insertItem( new uiAction("&Dump all wells to file ..."), 3 );
    mnu.insertItem( new uiAction("&Add dumped wells from file ..."), 4 );
    const int mnuid = mnu.exec();
    if ( mnuid < 0 ) return;

    Strat::Layer& lay = *ls.layers()[layidx];
    if ( mnuid == 0 )
    {
	uiStratEditLayer dlg( this, lay, ls, true );
	if ( dlg.go() )
	    forceRedispAll( true );
    }
    else if ( mnuid == 3 || mnuid == 4 )
	doLayModIO( mnuid == 4 );
    else if ( mnuid == 2 )
    {
	const_cast<Strat::LayerModel&>(layerModel()).removeSequence( selidx );
	forceRedispAll( true );
    }
    else
    {

	uiDialog dlg( this, uiDialog::Setup( "Remove a layer",
		    BufferString("Remove '",lay.name(),"'"),"110.0.12") );
	uiGenInput* gi = new uiGenInput( &dlg, "Remove", BoolInpSpec(true,
		    "Only this layer","All layers with this ID") );
	if ( dlg.go() )
	    removeLayers( ls, layidx, !gi->getBoolValue() );
    }
}


void uiStratSimpleLayerModelDisp::doLayModIO( bool foradd )
{
    if ( doLayerModelIO(foradd) )
	forceRedispAll( true );
}


void uiStratSimpleLayerModelDisp::removeLayers( Strat::LayerSequence& seq,
					int layidx, bool doall )
{
    if ( !doall )
    {
	delete seq.layers().removeSingle( layidx );
	seq.prepareUse();
    }
    else
    {
	const Strat::LeafUnitRef& lur = seq.layers()[layidx]->unitRef();
	for ( int ils=0; ils<layerModel().size(); ils++ )
	{
	    Strat::LayerSequence& ls = const_cast<Strat::LayerSequence&>(
						layerModel().sequence( ils ) );
	    bool needprep = false;
	    for ( int ilay=0; ilay<ls.layers().size(); ilay++ )
	    {
		const Strat::Layer& lay = *ls.layers()[ilay];
		if ( &lay.unitRef() == &lur )
		{
		    delete ls.layers().removeSingle( ilay );
		    ilay--; needprep = true;
		}
	    }
	    if ( needprep )
		ls.prepareUse();
	}
    }

    forceRedispAll( true );
}


void uiStratSimpleLayerModelDisp::doubleClicked( CallBacker* )
{
    const int selidx = getClickedModelNr();
    if ( selidx < 0 ) return;

    // Should we do something else than edit?
    handleRightClick(selidx);
}


void uiStratSimpleLayerModelDisp::forceRedispAll( bool modeledited )
{
    reDrawCB( 0 );
    if ( modeledited )
	modelEdited.trigger();
}


void uiStratSimpleLayerModelDisp::reDrawCB( CallBacker* )
{
    layerModel().prepareUse();
    if ( layerModel().isEmpty() )
    {
	if ( !emptyitm_ )
	    emptyitm_ = scene().addItem( new uiTextItem( "<---empty--->",
					 mAlignment(HCenter,VCenter) ) );
	emptyitm_->setPenColor( Color::Black() );
	emptyitm_->setPos( uiPoint( gv_->width()/2, gv_->height() / 2 ) );
	return;
    }

    delete emptyitm_; emptyitm_ = 0;
    doDraw();
}


void uiStratSimpleLayerModelDisp::setZoomBox( const uiWorldRect& wr )
{
    if ( !zoomboxitm_ )
    {
	zoomboxitm_ = scene().addItem( new uiRectItem );
	zoomboxitm_->setPenStyle( LineStyle(LineStyle::Dot,3,Color::Black()) );
	zoomboxitm_->setZValue( 100 );
    }

    if ( mIsUdf(wr.left()) )
    {
	zoomwr_.setLeft( mUdf(double) );
	getBounds();
    }
    else
    {
	// provided rect is always in system [0.5,N+0.5]
	zoomwr_.setLeft( wr.left() + .5 );
	zoomwr_.setRight( wr.right() + .5 );
	Interval<float> zrg( (float)wr.bottom(), (float)wr.top() );
    	mGetDispZrg(zrg,dispzrg);
	zoomwr_.setTop( dispzrg.start );
	zoomwr_.setBottom( dispzrg.stop );
    }
    updZoomBox();
    if ( showzoomed_ )
	forceRedispAll();
}


float uiStratSimpleLayerModelDisp::getDisplayZSkip() const
{
    if ( layerModel().isEmpty() ) return 0;
    return layerModel().sequence(0).startDepth();
}


void uiStratSimpleLayerModelDisp::updZoomBox()
{
    if ( zoomwr_.width() < 0.001 || !zoomboxitm_ || !xax_ )
	{ if ( zoomboxitm_ ) zoomboxitm_->setVisible( false ); return; }
    if ( mIsUdf(zoomwr_.left()) )
	getBounds();

    const int xpix = xax_->getPix( (float)zoomwr_.left() );
    const int ypix = yax_->getPix( (float)zoomwr_.top() );
    const int wdth = xax_->getPix( (float)zoomwr_.right() ) - xpix;
    const int hght = yax_->getPix( (float)zoomwr_.bottom() ) - ypix;
    zoomboxitm_->setRect( xpix, ypix, wdth, hght );
    zoomboxitm_->setVisible( !showzoomed_ );
}


void uiStratSimpleLayerModelDisp::modelChanged()
{
    forceRedispAll();
}



#define mStartLayLoop(chckdisp,perseqstmt) \
    const int nrseqs = layerModel().size(); \
    for ( int iseq=0; iseq<nrseqs; iseq++ ) \
    { \
	if ( chckdisp && !isDisplayedModel(iseq) ) continue; \
	const float lvldpth = lvldpths_[iseq]; \
	if ( flattened_ && mIsUdf(lvldpth) ) continue; \
	int layzlvl = 0; \
	const Strat::LayerSequence& seq = layerModel().sequence( iseq ); \
	const int nrlays = seq.size(); \
	perseqstmt; \
	for ( int ilay=0; ilay<nrlays; ilay++ ) \
	{ \
	    layzlvl++; \
	    const Strat::Layer& lay = *seq.layers()[ilay]; \
	    float z0 = lay.zTop(); if ( flattened_ ) z0 -= lvldpth; \
	    float z1 = lay.zBot(); if ( flattened_ ) z1 -= lvldpth; \
	    const float val = \
	       getLayerPropValue(lay,seq.propertyRefs()[dispprop_],dispprop_); \

#define mEndLayLoop() \
	} \
    }


void uiStratSimpleLayerModelDisp::getBounds()
{
    lvldpths_.erase();
    const Strat::Level* lvl = tools_.selStratLevel();
    for ( int iseq=0; iseq<layerModel().size(); iseq++ )
    {
	const Strat::LayerSequence& seq = layerModel().sequence( iseq );
	if ( !lvl || seq.isEmpty() )
	    { lvldpths_ += mUdf(float); continue; }

	const int posidx = seq.positionOf( *lvl );
	float zlvl = mUdf(float);
	if ( posidx >= seq.size() )
	    zlvl = seq.layers()[seq.size()-1]->zBot();
	else if ( posidx >= 0 )
	    zlvl = seq.layers()[posidx]->zTop();
	lvldpths_ += zlvl;
    }

    Interval<float> zrg(mUdf(float),mUdf(float)), vrg(mUdf(float),mUdf(float));
    mStartLayLoop( false,  )
#	define mChckBnds(var,op,bnd) \
	if ( (mIsUdf(var) || var op bnd) && !mIsUdf(bnd) ) \
	    var = bnd
	mChckBnds(zrg.start,>,z0);
	mChckBnds(zrg.stop,<,z1);
	mChckBnds(vrg.start,>,val);
	mChckBnds(vrg.stop,<,val);
    mEndLayLoop()
    if ( mIsUdf(zrg.start) )
	zrg_ = Interval<float>( 0, 1 );
    else
	zrg_ = zrg;
    vrg_ = mIsUdf(vrg.start) ? Interval<float>(0,1) : vrg;

    if ( mIsUdf(zoomwr_.left()) )
    {
	zoomwr_.setLeft( 1 );
	zoomwr_.setRight( nrseqs+1 );
	mGetDispZrg(zrg_,dispzrg);
	zoomwr_.setTop( dispzrg.stop );
	zoomwr_.setBottom( dispzrg.start );
    }
}


int uiStratSimpleLayerModelDisp::getXPix( int iseq, float relx ) const
{
    const float margin = 0.05f;
    relx = (1-margin) * relx + margin * .5f; // get relx between 0.025 and 0.975
    relx *= dispeach_;
    return xax_->getPix( iseq + 1 + relx );
}


bool uiStratSimpleLayerModelDisp::isDisplayedModel( int iseq ) const
{
    if ( iseq % dispeach_ )
	return false;

    if ( showzoomed_ )
    {
	const int xpix0 = getXPix( iseq, 0 );
	const int xpix1 = getXPix( iseq, 1 );
	if ( xax_->getVal(xpix1) > zoomwr_.right()
	  || xax_->getVal(xpix0) < zoomwr_.left() )
	    return false;
    }
    return true;
}


int uiStratSimpleLayerModelDisp::totalNrLayersToDisplay() const
{
    const int nrseqs = layerModel().size();
    int ret = 0;
    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	if ( isDisplayedModel(iseq) )
	    ret += layerModel().sequence(iseq).size();
    }
    return ret;
}


void uiStratSimpleLayerModelDisp::doDraw()
{
    dispprop_ = tools_.selPropIdx();
    selectedlevel_ = tools_.selLevelIdx();
    dispeach_ = tools_.dispEach();
    showzoomed_ = tools_.dispZoomed();
    uselithcols_ = tools_.dispLith();
    selectedcontent_ = layerModel().refTree().contents()
				.getByName(tools_.selContent());
    allcontents_ = FixedString(tools_.selContent()) == sKey::All();

    getBounds();

    xax_->updateDevSize(); yax_->updateDevSize();
    if ( !showzoomed_ )
    {
	xax_->setBounds( Interval<float>(1,mCast(float,layerModel().size() )));
    	mGetDispZrg(zrg_,dispzrg);
	yax_->setBounds( Interval<float>(dispzrg.stop,dispzrg.start) );
    }
    else
    {
	xax_->setBounds( Interval<float>((float)zoomwr_.left(),
						(float)zoomwr_.right()) );
	yax_->setBounds( Interval<float>((float)zoomwr_.top(),
		    	 		 (float)zoomwr_.bottom()) );
    }

    yax_->plotAxis(); xax_->plotAxis();
    if ( vrg_.width() == 0 ) 
	{ vrg_.start -= 1; vrg_.stop += 1; }
    const float vwdth = vrg_.width();
    float zfac = 1; mGetDispZ( zfac );
    bool dofill = fillmdls_ || totalNrLayersToDisplay() < cMaxNrLayers4RectDisp;
    float prevrelx = mUdf(float);

    int lineitmidx=0, rectitmidx=0;

    mStartLayLoop( true, int lastdrawnypix = mUdf(int) )


	float dispz0 = z0; float dispz1 = z1;
	mGetConvZ( dispz0, zfac ); mGetConvZ( dispz1, zfac );
	if ( showzoomed_ )
	{
	    if ( dispz0 > zoomwr_.top() || dispz1 < zoomwr_.bottom() )
		continue;
	    if ( dispz1 > zoomwr_.top() )
		dispz1 = (float)zoomwr_.top();
	    if ( dispz0 < zoomwr_.bottom() )
		dispz0 = (float)zoomwr_.bottom();
	}

	const int ypix1 = yax_->getPix( dispz1 );
	if ( mIsUdf(val) )
	    { lastdrawnypix = ypix1; continue; }

	const int ypix0 = mIsUdf(lastdrawnypix) ? yax_->getPix( dispz0 )
						: lastdrawnypix + 1;
	if ( ypix0 >= ypix1 )
	    continue;

	const Color laycol = lay.dispColor( uselithcols_ );
	bool mustannotcont = false;
	if ( !lay.content().isUnspecified() )
	    mustannotcont = allcontents_
		|| (selectedcontent_ && lay.content() == *selectedcontent_);
	const Color pencol = mustannotcont ? lay.content().color_ : laycol;
	const bool needrect = dofill || mustannotcont;

	lastdrawnypix = ypix1;
	const float relx = (val-vrg_.start) / vwdth;
	if ( mIsUdf(prevrelx) ) prevrelx = relx;
	const int xpix0 = getXPix( iseq, needrect ? 0 : prevrelx );
	const int xpix1 = getXPix( iseq, relx );
	prevrelx = relx;

#	define mHandleItem(it) \
	    it->setPenColor( pencol ); \
	    if ( pencol != laycol ) \
		it->setPenStyle( LineStyle(LineStyle::Solid,2,pencol) ); \
	    it->setZValue( layzlvl );

	if ( dofill || mustannotcont )
	{
	    uiRectItem* rectitm = 0;
	    if ( rectitmidx < logblckrectitms_.size() )
	    {
		mDynamicCast(uiRectItem*,rectitm,logblckrectitms_[rectitmidx]);
		rectitm->show();
	    }
	    else
	    {
		rectitm = scene().addItem( new uiRectItem() );
		logblckrectitms_ += rectitm;
	    }

	    if ( rectitm )
	    {
		rectitmidx++;
		rectitm->setRect( xpix0, ypix0, xpix1-xpix0+1, ypix1-ypix0+1 );
		mHandleItem( rectitm );
		rectitm->setFillColor( laycol );
		if ( mustannotcont )
		    rectitm->setFillPattern( lay.content().pattern_ );
		else
		{
		    FillPattern fp; fp.setFullFill();
		    rectitm->setFillPattern( fp );
		}
	    }
	}
	else
	{
#	define mAddLineItem(lineitm) \
	    uiLineItem* lineitm = 0; \
	    if ( lineitmidx < logblcklineitms_.size() ) \
	    { \
		mDynamicCast(uiLineItem*,lineitm,logblcklineitms_[lineitmidx]) \
		lineitm->show(); \
	    } \
	    else \
	    { \
		lineitm = scene().addItem( new uiLineItem() ); \
		lineitm->show(); \
		logblcklineitms_ += lineitm; \
	    } \
	    if ( lineitm ) \
	    { \
		lineitmidx++; \
		mHandleItem( lineitm ); \
	    }

	    mAddLineItem( lineitm1 );
	    mAddLineItem( lineitm2 );
	    if ( lineitm1 ) lineitm1->setLine( xpix0, ypix0, xpix1, ypix0 );
	    if ( lineitm2 ) lineitm2->setLine( xpix1, ypix0, xpix1, ypix1 );
	}


    mEndLayLoop()

    for ( int idx=rectitmidx; idx<logblckrectitms_.size(); idx++ )
	logblckrectitms_[idx]->hide();
    for ( int idx=lineitmidx; idx<logblcklineitms_.size(); idx++ )
	logblcklineitms_[idx]->hide();

    drawLevels();
    drawSelectedSequence();
    displayFRText();
    updZoomBox();
}


void uiStratSimpleLayerModelDisp::drawLevels()
{
    if ( layerModel().isEmpty() )
	return;

    int itmidx = 0;
    lvlcol_ = tools_.selLevelColor();
    for ( int iseq=0; iseq<lvldpths_.size(); iseq++ )
    {
	float zlvl = lvldpths_[iseq];
	if ( mIsUdf(zlvl) )
	    continue;

	mGetDispZ(zlvl);
	const int ypix = yax_->getPix( flattened_ ? 0 : zlvl );
	const int xpix1 = getXPix( iseq, 0 );
	const int xpix2 = getXPix( iseq, 1 );

	uiLineItem* it = 0;
	if ( itmidx < lvlitms_.size() )
	{
	    mDynamicCast(uiLineItem*,it,lvlitms_[itmidx])
	    if ( it ) it->show();
	}
	else
	{
	    it = scene().addItem( new uiLineItem() );
	    lvlitms_ += it;
	}

	if ( !it )
	    continue;
 
	it->setLine( uiPoint(xpix1,ypix), uiPoint(xpix2,ypix) );
	it->setPenStyle( LineStyle(LineStyle::Solid,2,lvlcol_) );
	it->setZValue( 999999 );
	itmidx++;
    }

    for ( int idx=itmidx; idx<lvlitms_.size(); idx++ )
	lvlitms_[idx]->hide();
}


void uiStratSimpleLayerModelDisp::doLevelChg()
{ reDrawCB( 0 ); }


void uiStratSimpleLayerModelDisp::drawSelectedSequence()
{
    delete selseqitm_; selseqitm_ = 0;
    const int nrseqs = layerModel().size();
    if ( nrseqs < 1 || selseqidx_ > nrseqs || selseqidx_ < 0 ) return;

    const int ypix1 = yax_->getPix( yax_->range().start );
    const int ypix2 = yax_->getPix( yax_->range().stop );
    const float xpix1 = (float)getXPix( selseqidx_, 0 );
    const float xpix2 = (float)getXPix( selseqidx_, 1 );
    const int midpix = (int)( xpix1 + ( xpix2 - xpix1 ) /2 );

    uiLineItem* it = scene().addItem(
	new uiLineItem( uiPoint(midpix,ypix1), uiPoint(midpix,ypix2) ) );

    it->setPenStyle( LineStyle(LineStyle::Dot,2,Color::Black()) );
    it->setZValue( 9999999 );
    selseqitm_ = it;
}
