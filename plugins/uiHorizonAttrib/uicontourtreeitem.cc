/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uicontourtreeitem.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "isocontourtracer.h"
#include "axislayout.h"
#include "mousecursor.h"
#include "polygon.h"
#include "survinfo.h"
#include "hiddenparam.h"

#include "uibutton.h"
#include "uidialog.h"
#include "uiempartserv.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiprogressbar.h"
#include "uiodapplmgr.h"
#include "uioddisplaytreeitem.h"
#include "uiodscenemgr.h"
#include "uisellinest.h"
#include "uistatusbar.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uitable.h"
#include "uifiledlg.h"

#include "viscoord.h"
#include "visdrawstyle.h"
#include "vishorizondisplay.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistext.h"
#include "vistransform.h"
#include "zaxistransform.h"

#include <fstream>

static const int cMinNrNodesForLbl = 25;

class uiContourParsDlg : public uiDialog
{
public:
uiContourParsDlg( uiParent* p, const char* attrnm, const Interval<float>& rg,
       		  const StepInterval<float>& intv, const LineStyle& ls,
		  int sceneid )
    : uiDialog(p,Setup("Contour Display Options",mNoDlgTitle,"104.3.2")
		 .nrstatusflds(1))
    , rg_(rg)
    , contourintv_(intv)
    , propertyChanged(this)
    , intervalChanged(this)
{
    BufferString zvalstr( uiContourTreeItem::sKeyZValue() );
    iszval_ = zvalstr == attrnm;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneid))

    if ( iszval_ )
    {
	zfac_ = mCast( float, scene->zDomainInfo().userFactor() );
	rg_.scale( zfac_ );
	contourintv_.scale( zfac_ );
    }

    BufferString lbltxt = "Total ";
    lbltxt += attrnm;
    lbltxt += " Range ";
    if ( iszval_ )
	lbltxt += scene->zDomainInfo().unitStr(true);
    lbltxt += ": "; lbltxt += rg_.start;
    lbltxt += " - "; lbltxt += rg_.stop;
    uiLabel* lbl = new uiLabel( this, lbltxt.buf() );

    lbltxt = "Contour range ";
    if ( iszval_ )
	lbltxt += scene->zDomainInfo().unitStr(true);

    intvfld_ = new uiGenInput(this,lbltxt,FloatInpIntervalSpec(contourintv_));
    intvfld_->valuechanged.notify( mCB(this,uiContourParsDlg,intvChanged) );
    intvfld_->attach( leftAlignedBelow, lbl );
    uiPushButton* applybut = new uiPushButton( this, "Apply", true );
    applybut->attach( rightTo, intvfld_ );
    applybut->activated.notify( mCB(this,uiContourParsDlg,applyCB) );

    uiSelLineStyle::Setup lssu; lssu.drawstyle(false);
    lsfld_ = new uiSelLineStyle( this, ls, lssu );
    lsfld_->attach( alignedBelow, intvfld_ );
    lsfld_->changed.notify( mCB(this,uiContourParsDlg,dispChanged) );

    showlblsfld_ = new uiCheckBox( this, "Show labels" );
    showlblsfld_->activated.notify( mCB(this,uiContourParsDlg,dispChanged) );
    showlblsfld_->attach( alignedBelow, lsfld_ );

    intvChanged( 0 );
}

const LineStyle& getLineStyle() const
{ return lsfld_->getStyle(); }

StepInterval<float> getContourInterval() const
{
    StepInterval<float> res = intvfld_->getFStepInterval();
    if ( iszval_ )
	res.scale( 1.0f/zfac_ );
    
    return res;
}

void setShowLabels( bool yn )
{ showlblsfld_->setChecked( yn ); }

bool showLabels() const
{ return showlblsfld_->isChecked(); }

    Notifier<uiContourParsDlg>	propertyChanged;
    Notifier<uiContourParsDlg>	intervalChanged;

protected:

void applyCB( CallBacker* )
{ intervalChanged.trigger(); }

void dispChanged( CallBacker* )
{ propertyChanged.trigger(); }

void intvChanged( CallBacker* cb )
{
    StepInterval<float> intv = intvfld_->getFStepInterval();
    if ( intv.start < rg_.start || intv.start > rg_.stop )
	intvfld_->setValue( contourintv_.start, 0 );
    if ( intv.stop < rg_.start || intv.stop > rg_.stop )
	intvfld_->setValue( contourintv_.stop, 1 );

    if ( intv.step <= 0 || intv.step > rg_.width() )
    {
	if ( cb )
	    uiMSG().error( "Please specify a valid value for step" );

	intvfld_->setValue( contourintv_.step, 2 );
	return;
    }

    intv = intvfld_->getFStepInterval();
    contourintv_.step = intv.step;

    BufferString txt( "Number of contours: ", intv.nrSteps()+1 );
    toStatusBar( txt );
}

    Interval<float>	rg_;
    StepInterval<float>	contourintv_;
    uiGenInput*		intvfld_;
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		showlblsfld_;
    float		zfac_;
    bool		iszval_;
};


class visContourLabels : public visBase::VisualObjectImpl
{
public:

visContourLabels()
    : VisualObjectImpl(false)
    , transformation_(0)
{}

~visContourLabels()
{
    if ( transformation_ )
	transformation_->unRef();
}

void addLabel( visBase::Text2* label )
{
    label->setDisplayTransformation( transformation_ );
    addChild( label->getInventorNode() );
}

void setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ ) transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ ) transformation_->ref();
}

    const mVisTrans*	transformation_;
};



const char* uiContourTreeItem::sKeyContourDefString()
{ return "Countour Display"; }

const char* uiContourTreeItem::sKeyZValue()
{ return "Z Values"; }

void uiContourTreeItem::initClass()
{ uiODDataTreeItem::factory().addCreator( create, 0 ); }

HiddenParam<uiContourTreeItem, MenuItem*> areamenuitem( 0 );
HiddenParam<uiContourTreeItem, TypeSet<double>*> areavals( 0 );




uiContourTreeItem::uiContourTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , optionsmenuitem_( "Properties ..." )
    , lines_( 0 )
    , drawstyle_( 0 )
    , material_(0)
    , labelgrp_(0)
    , linewidth_(1)
    , arr_(0)
    , rg_(mUdf(float),-mUdf(float))
    , zshift_(mUdf(float))
    , color_(0,0,0)
    , showlabels_(true)
{
    optionsmenuitem_.iconfnm = "disppars";

    ODMainWin()->applMgr().visServer()->removeAllNotifier().notify(
	    mCB(this,uiContourTreeItem,visClosingCB) );

    areamenuitem.setParam( this, new MenuItem( "Contour areas" ) );
    areavals.setParam( this, new TypeSet<double> );

    // No icon before we find a painter
    // areaMenuItem().iconfnm = "contourarea";
}


uiContourTreeItem::~uiContourTreeItem()
{
    delete arr_;

    delete areamenuitem.getParam( this );
    areamenuitem.removeParam( this );

    delete areavals.getParam( this );
    areavals.removeParam( this );

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
		    applMgr()->visServer()->getObject(displayID()))
    if ( hd )
	hd->getMovementNotifier()->remove(mCB(this,uiContourTreeItem,checkCB));

    applMgr()->visServer()->removeAllNotifier().remove(
		mCB(this,uiContourTreeItem,visClosingCB) );

    if ( lines_ || drawstyle_ )
	pErrMsg("prepareForShutdown not run");

    if ( !parent_ )
	return;

    parent_->checkStatusChange()->remove(mCB(this,uiContourTreeItem,checkCB));
}


MenuItem& uiContourTreeItem::areaMenuItem()
{
    return *areamenuitem.getParam( this );
}


TypeSet<double>& uiContourTreeItem::areas()
{
    return *areavals.getParam( this );
}


const TypeSet<double>& uiContourTreeItem::areas() const
{
    return *areavals.getParam( this );
}


void uiContourTreeItem::prepareForShutdown()
{
    visClosingCB( 0 );
}


bool uiContourTreeItem::init()
{
    if ( !uiODDataTreeItem::init() )
	return false;

    uitreeviewitem_->setChecked( true );
    parent_->checkStatusChange()->notify(mCB(this,uiContourTreeItem,checkCB));
    return true;
}


uiODDataTreeItem* uiContourTreeItem::create( const Attrib::SelSpec& as,
					     const char* parenttype )
{
    BufferString defstr = as.defString();
    return defstr == sKeyContourDefString() ? new uiContourTreeItem(parenttype)
					    : 0;
}


void uiContourTreeItem::checkCB(CallBacker*)
{
    bool newstatus = uitreeviewitem_->isChecked();
    if ( newstatus && parent_ )
	newstatus = parent_->isChecked();

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
		    applMgr()->visServer()->getObject(displayID()))
    const bool display = newstatus && hd && !hd->getOnlyAtSectionsDisplay();
    
    if ( lines_ ) lines_->turnOn( display );
    if ( labelgrp_ ) labelgrp_->turnOn( display && showlabels_ );

    updateZShift();
}


void uiContourTreeItem::visClosingCB( CallBacker* )
{
    removeAll();
}


void uiContourTreeItem::removeAll()
{
    if ( lines_ )
    {
	applMgr()->visServer()->removeObject( lines_, sceneID() );
	lines_->unRef();
	lines_ = 0;
    }

    if ( drawstyle_ )
    {
	drawstyle_->unRef();
	drawstyle_ = 0;
    }

    if ( material_ )
    {
	material_->unRef();
	material_ = 0;
    }

    removeLabels();
}


void uiContourTreeItem::removeLabels()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( labelgrp_ )
    {
	visserv->removeObject( labelgrp_, sceneID() );
	labelgrp_->unRef();
	labelgrp_ = 0;
    }

    deepUnRef( labels_ );
    labels_.erase();
}


void uiContourTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDataTreeItem::createMenu( menu, istb );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_,
		      &optionsmenuitem_, lines_, false );

    mAddMenuOrTBItem( istb, menu, &displaymnuitem_,
                     &areaMenuItem(), lines_ && areas().size(), false );
}

const char* areaString()
{
    return SI().xyInFeet() ? "Area (sqft)" : " Area (m^2)";
}


void uiContourTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);

    if ( mnuid==-1 || menu->isHandled() || !lines_ )
        return;

    if ( mnuid==optionsmenuitem_.id )
    {
        menu->setIsHandled( true );
        Interval<float> range( rg_.start, rg_.stop );
        range += Interval<float>( zshift_, zshift_ );
        StepInterval<float> oldintv( contourintv_ );
        oldintv += Interval<float>( zshift_, zshift_ );
        uiContourParsDlg dlg( ODMainWin(), attrnm_, range, oldintv,
                              LineStyle(LineStyle::Solid,linewidth_,color_),
                              sceneID() );
        if ( labelgrp_ )
            dlg.setShowLabels( labelgrp_->isOn() );
        dlg.propertyChanged.notify( mCB(this,uiContourTreeItem,propChangeCB) );
        dlg.intervalChanged.notify( mCB(this,uiContourTreeItem,intvChangeCB) );
        const bool res = dlg.go();
        dlg.propertyChanged.remove( mCB(this,uiContourTreeItem,propChangeCB) );
        dlg.intervalChanged.remove( mCB(this,uiContourTreeItem,intvChangeCB) );
        if ( !res ) return;

        StepInterval<float> newintv = dlg.getContourInterval();
        updateContours( newintv );
    }

    if ( mnuid==areaMenuItem().id )
    {
        menu->setIsHandled( true );

        TypeSet<float> zvals, areas;
        getZVSAreaValues( zvals, areas );

        uiDialog dlg( ODMainWin(), uiDialog::Setup("Countour areas", 0,
                    				   mNoHelpID ) );
        dlg.setCancelText( 0 );

        RefMan<visSurvey::Scene> mDynamicCast( visSurvey::Scene*, scene,
                                applMgr()->visServer()->getObject(sceneID()) );

        if ( !scene ) { pErrMsg("No scene"); return; }

        const ZDomain::Info& zinfo = scene->zDomainInfo();

	uiTable* table = new uiTable( &dlg, uiTable::Setup(areas.size(),2),
                                     "Area table");

        const BufferString zdesc( zinfo.userName(), " ", zinfo.unitStr(true) );
        table->setColumnLabel( 0, zdesc.buf() );


        table->setColumnLabel( 1, areaString() );

        for ( int idx=0; idx<areas.size(); idx++ )
        {
            table->setText( RowCol(idx,0),
                           toString( zvals[idx] * zinfo.userFactor() ) );
            table->setText( RowCol(idx,1), toString( areas[idx] ) );
        }

        uiPushButton* button = new uiPushButton( &dlg, "Save &as",
                            mCB(this,uiContourTreeItem,saveAreasAsCB), true );
        button->attach( leftAlignedBelow, table );
        
        dlg.go();
    }
}


void uiContourTreeItem::getZVSAreaValues( TypeSet<float>& zvals,
                                         TypeSet<float>& areavals ) const
{
    for ( int idx=0; idx<areas().size(); idx++ )
    {
        if ( !mIsUdf(areas()[idx]))
        {
            zvals += contourintv_.atIndex(idx);
            areavals += areas()[idx];
        }
    }
}


void uiContourTreeItem::saveAreasAsCB(CallBacker*)
{
    uiFileDialog dlg( ODMainWin(), 0, "*.txt",
                     "Select file to store contour areas" );
    if ( dlg.go() )
    {
        std::ofstream stream( dlg.fileName() );
        RefMan<visSurvey::Scene> mDynamicCast( visSurvey::Scene*, scene,
                                              applMgr()->visServer()->getObject(sceneID()) );

        if ( !scene ) { pErrMsg("No scene"); return; }

        const ZDomain::Info& zinfo = scene->zDomainInfo();

        stream << BufferString(zinfo.userName(), " ", zinfo.unitStr(true))
        << '\t' << areaString() << '\n';

        TypeSet<float> zvals, areavalues;
        getZVSAreaValues( zvals, areavalues );

        for ( int idx=0; idx<areas().size(); idx++ )
        {
            stream << zvals[idx] * zinfo.userFactor() << '\t'
                    << areavalues[idx] << '\n';
        }

        if ( stream.bad() )
        {
            BufferString errmsg( "Could not save file ", dlg.fileName() );

            uiMSG().error( errmsg.buf() );
        }
        else
        {
            uiMSG().message( BufferString( "Area table saved as ",
                                          dlg.fileName(), ".").buf());
        }
    }
}


void uiContourTreeItem::updateContours( const StepInterval<float>& newintv )
{
    StepInterval<float> oldintv = contourintv_;
    oldintv += Interval<float>( zshift_, zshift_ );
    const bool intvchgd = !mIsEqual(newintv.start,oldintv.start,1e-4) ||
			  !mIsEqual(newintv.stop,oldintv.stop,1e-4) ||
			  !mIsEqual(newintv.step,oldintv.step,1e-4);

    if ( intvchgd )
    {
	contourintv_ = newintv;
	contourintv_ += Interval<float>( -zshift_, -zshift_ );
	createContours();
    }
}


void uiContourTreeItem::intvChangeCB( CallBacker* cb )
{
    mDynamicCastGet(uiContourParsDlg*,dlg,cb);
    if ( !dlg ) return;

    StepInterval<float> newintv = dlg->getContourInterval();
    updateContours( newintv );
}


void uiContourTreeItem::propChangeCB( CallBacker* cb )
{
    mDynamicCastGet(uiContourParsDlg*,dlg,cb);
    if ( !dlg || !lines_ ) return;

    LineStyle ls( dlg->getLineStyle() );
    drawstyle_->setLineStyle( ls );
    material_->setColor( ls.color_ );
    color_ = ls.color_;
    linewidth_ = ls.width_;

    if ( labelgrp_ && lines_ )
    {
	showlabels_ = dlg->showLabels();
	labelgrp_->turnOn( lines_->isOn() && showlabels_ );
    }
}


bool uiContourTreeItem::computeContours( const Array2D<float>& field,
					 const StepInterval<int>& rowrg,
					 const StepInterval<int>& colrg )
{
    if ( mIsUdf(rg_.start) )
    {
	for ( int idx=0; idx<field.info().getSize(0); idx++ )
	{
	    for ( int idy=0; idy<field.info().getSize(1); idy++ )
	    {
		const float val = field.get( idx, idy );
		if ( !mIsUdf(val) ) rg_.include( val, false );
	    }
	}
	
	if ( mIsUdf(rg_.start) )
	    return false;

	AxisLayout<float> al( rg_ );
	SamplingData<float> sd = al.sd_;
	sd.step /= 5;
	const float offset = ( sd.start - rg_.start ) / sd.step;
	if ( offset < 0 || offset > 1 )
	{
	    const int nrsteps = mNINT32( floor(offset) );
	    sd.start -= nrsteps * sd.step;
	}

	contourintv_.start = sd.start;
	contourintv_.stop = rg_.stop;
	contourintv_.step = sd.step;
	const int nrsteps = contourintv_.nrSteps();
	contourintv_.stop = sd.start + nrsteps*sd.step;
    }

    if ( contourintv_.step <= 0 )
	return false;

    return true;
}


Array2D<float>* uiContourTreeItem::getDataSet( visSurvey::HorizonDisplay* hd )
{
    EM::ObjectID emid = hd->getObjectID();
    mDynamicCastGet(EM::Horizon3D*,hor,EM::EMM().getObject(emid));
    if ( !hor ) return 0;

    EM::SectionID sid = hor->sectionID( 0 );
    if ( attrnm_ == uiContourTreeItem::sKeyZValue() )
    {
	Array2D<float>* arr = hor->geometry().sectionGeometry(sid)->getArray();
	if ( hd->getZAxisTransform() )
	    arr = hor->createArray2D( sid, hd->getZAxisTransform() );
	return arr;
    }

    const int dataid = applMgr()->EMServer()->loadAuxData( hor->id(), attrnm_ );
    Array2D<float>* arr = hor->auxdata.createArray2D( dataid, sid );
    return arr;
}


class uiContourProgressWin : public uiMainWin
{
public:

uiContourProgressWin( uiParent* p, const int totalnr )
    : uiMainWin(p,uiMainWin::Setup("Creating contours"))
{
    setPrefWidth( 100 );
    setPrefHeight( 10 );
    progbar_ = new uiProgressBar( this, "Contours", totalnr );
    progbar_->attach( topBorder, 10 );
    progbar_->setHSzPol( uiObject::WideMax );
}

void setProgress( int prog )
{ progbar_->setProgress( prog ); }

int progress() const
{ return progbar_->progress(); }

void setText( const char* txt )
{ statusBar()->setLabelTxt( 0, txt ); } 

uiProgressBar*		progbar_;

};


void uiContourTreeItem::createContours()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
		    visserv->getObject(displayID()))
    if ( !hd )
	return;

    hd->getMovementNotifier()->notifyIfNotNotified(
		mCB(this,uiContourTreeItem,checkCB) );

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    StepInterval<int> rowrg = hd->geometryRowRange();
    StepInterval<int> colrg = hd->geometryColRange();

    createLines();
    if ( !lines_ ) return;

    EM::ObjectID emid = hd->getObjectID();
    mDynamicCastGet(EM::Horizon3D*,hor,EM::EMM().getObject(emid));
    if ( !hor ) return;


    Array2D<float>* field = getDataSet( hd );
    if ( !field ) return;

    IsoContourTracer ictracer( *field );
    ictracer.setSampling( rowrg, colrg );
    if ( !computeContours(*field,rowrg,colrg) )
	return;

    const Coord3 trans = applMgr()->visServer()->getTranslation( displayID() );
    zshift_ = (float) trans.z;

    const char* fmt = SI().zIsTime() ? "%g" : "%f";
    int cii = 0;
    float contourval = contourintv_.start;
    lines_->getCoordinates()->removeAfter( -1 );
    lines_->removeCoordIndexAfter( -1 );

    removeLabels();
    const float maxcontourval = mMIN(contourintv_.stop,rg_.stop);
   
    uiContourProgressWin progwin( ODMainWin(), contourintv_.nrSteps() );
    progwin.show();

    mDynamicCastGet(visSurvey::Scene*,scene, visserv->getObject(sceneID()));
    const ZAxisTransform* transform = scene ? scene->getZAxisTransform() : 0;
    const float fac = mCast( float, scene->zDomainInfo().userFactor() );
    areas().setEmpty();

    while ( contourval < maxcontourval+mDefEps )
    {
        double area = 0;
	ManagedObjectSet<ODPolygon<float> > isocontours;
	ictracer.getContours( isocontours, contourval, false );
	for ( int cidx=0; cidx<isocontours.size(); cidx++ )
	{
	    const ODPolygon<float>& ic = *isocontours[cidx];
	    char buf[255];
	    for ( int vidx=0; vidx<ic.size(); vidx++ )
	    {
		const Geom::Point2D<float> vertex = ic.getVertex( vidx );
		BinID vrtxbid( rowrg.snap(vertex.x), colrg.snap(vertex.y) );
		float zval = hor->getZ( vrtxbid );
		if ( transform )
		    transform->transform( vrtxbid,
					SamplingData<float>(zval,1), 1, &zval );
		if ( mIsUdf(zval) )
		{
		    lines_->setCoordIndex( cii++, -1 );
		    continue;
		}

		Coord vrtxcoord = SI().binID2Coord().transform( vrtxbid );
		const Coord3 pos( vrtxcoord, zval+zshift_ );
		const int posidx = lines_->getCoordinates()->addPos( pos );
		lines_->setCoordIndex( cii++, posidx );
		const float labelval = attrnm_==uiContourTreeItem::sKeyZValue()
			? (contourval+zshift_) * fac : contourval;
		if ( ic.size() > cMinNrNodesForLbl && vidx == ic.size()/2 )
		    addText( pos, getStringFromFloat(fmt, labelval, buf) );
	    }


	    if ( ic.isClosed() )
	    {
		const int posidx = lines_->getCoordIndex( cii-ic.size() );
		lines_->setCoordIndex( cii++, posidx );

	    }

            if ( !mIsUdf(area) && ic.isClosed() )
                area += ic.area();
            else
                area = mUdf(double);


	    lines_->setCoordIndex( cii++, -1 );
	}

        areas() += area;

	progwin.setProgress( progwin.progress() + 1 );
	contourval += contourintv_.step;
    }

    progwin.close();
    lines_->removeCoordIndexAfter( cii-1 );
    if ( hd->getZAxisTransform() )
	delete field;

    bool validfound = false;
    for ( int idx=0; idx<areas().size(); idx++ )
    {
        if ( !mIsUdf(areas()[idx]))
        {
            validfound = true;
            break;
        }
    }

    if ( !validfound )
        areas().erase();

    if ( labelgrp_ )
	labelgrp_->turnOn( showlabels_ );
}


void uiContourTreeItem::createLines()
{
    if ( lines_ )
	return;

    lines_ = visBase::IndexedPolyLine::create();
    lines_->ref();
    applMgr()->visServer()->addObject( lines_, sceneID(), false );
    if ( !drawstyle_ )
    {
	drawstyle_ = visBase::DrawStyle::create();
	drawstyle_->ref();
	lines_->insertNode( drawstyle_->getInventorNode() );
    }

    if ( !material_ )
    {
	material_ = visBase::Material::create();
	material_->setColor( color_ );
	material_->ref();
	lines_->setMaterial( material_ );
    }
}


void uiContourTreeItem::addText( const Coord3& pos, const char* txt )
{
    visBase::Text2* label = visBase::Text2::create();
    label->setText( txt );
    label->setPosition( pos );
    label->setFontData( FontData(12) );
    label->setMaterial( material_ );
    label->ref();
    if ( !labelgrp_ )
    {
	labelgrp_ = new visContourLabels;
	labelgrp_->ref();
	applMgr()->visServer()->addObject( labelgrp_, sceneID(), false );
    }

    labelgrp_->addLabel( label );
    labels_ += label;
}


void uiContourTreeItem::updateColumnText( int col )
{
    uiODDataTreeItem::updateColumnText( col );
    if ( !col && !lines_ )
	createContours();

    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(const visSurvey::HorizonDisplay*,hd,
		    visserv->getObject(displayID()))
    if ( !hd || !lines_ || !labelgrp_ ) return;

    const bool solomode = visserv->isSoloMode();
    const bool turnon = !hd->getOnlyAtSectionsDisplay() &&
       ( (solomode && hd->isOn()) || (!solomode && hd->isOn() && isChecked()) );
    lines_->turnOn( turnon );
    labelgrp_->turnOn( turnon && showlabels_ );
}


BufferString uiContourTreeItem::createDisplayName() const
{
    BufferString nm( "Contours (" );
    nm += attrnm_; nm += ")";
    return nm;
}


void uiContourTreeItem::updateZShift()
{
    if ( !lines_ || mIsUdf(zshift_) )
	return;

    const Coord3 trans = applMgr()->visServer()->getTranslation( displayID() );
    const float deltaz = (float) (trans.z - zshift_);
    if ( !deltaz )
	return;

    for ( int idx=0; idx<lines_->getCoordinates()->size(true); idx++ )
    {
	if ( lines_->getCoordinates()->isDefined(idx) )
	{
	    Coord3 pos = lines_->getCoordinates()->getPos( idx );
	    pos.z += deltaz;
	    lines_->getCoordinates()->setPos( idx, pos );
	}
    }

    char buf[255];
    const char* fmt = SI().zIsTime() ? "%g" : "%f";

    for ( int idx=0; idx<labels_.size(); idx++ )
    {
	Coord3 pos = labels_[idx]->position();
	pos.z += deltaz;
	labels_[idx]->setPosition( pos );
	float labelval = toFloat( labels_[idx]->getText() );
	labelval += deltaz * SI().zDomain().userFactor(); 
	labels_[idx]->setText( getStringFromFloat(fmt, labelval, buf) );
    }

    zshift_ = (float) trans.z;
}
