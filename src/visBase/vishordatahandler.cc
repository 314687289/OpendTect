  /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : April 2013
-*/


#include "vishordatahandler.h"
#include "vishorizonsectiondef.h"
#include "visdata.h"

#include "vishorizonsection.h"
#include "zaxistransform.h"
#include "trckeyzsampling.h"
#include "binidvalue.h"
#include "binidsurface.h"
#include "datacoldef.h"
#include "posvecdataset.h"
#include "datapointset.h"


using namespace visBase;

HorizonSectionDataHandler::HorizonSectionDataHandler(
    const HorizonSection* hrsection )
    : horsection_( hrsection )
    , zaxistransformvoi_( -2 )
    , zaxistransform_( 0 )
{

}

HorizonSectionDataHandler::~HorizonSectionDataHandler()
{
    removeZTransform();
}

void HorizonSectionDataHandler::updateZAxisVOI()
{
    if( !horsection_ ) return;

    Geometry::BinIDSurface* geometry = horsection_->geometry_;

    if ( !geometry || zaxistransformvoi_==-1 )
	return;

    if ( !zaxistransform_ || !zaxistransform_->needsVolumeOfInterest() )
	return;

    TrcKeyZSampling cs;
    if ( horsection_->userchangedisplayrg_ )
	cs.hsamp_.set( horsection_->displayrrg_, horsection_->displaycrg_ );
    else
	cs.hsamp_.set( geometry->rowRange(), geometry->colRange() );

    TrcKeySamplingIterator iter( cs.hsamp_ );

    bool first = true;
    do
    {
	const BinID curpos( iter.curBinID() );
	const float depth = geometry->getKnot(RowCol(curpos),false).z_;
	if ( mIsUdf(depth) )
	    continue;

	if ( first )
	{
	    cs.zsamp_.start = cs.zsamp_.stop = depth;
	    first = false;
	}
	else
	    cs.zsamp_.include( depth );
    } while ( iter.next() );

    if ( first ) return;

    if ( zaxistransformvoi_==-2 )
	zaxistransformvoi_ = zaxistransform_->addVolumeOfInterest( cs, false );
    else
	zaxistransform_->setVolumeOfInterest( zaxistransformvoi_, cs, false );

}


void HorizonSectionDataHandler::setZAxisTransform( ZAxisTransform* zt )
{
    if ( zaxistransform_==zt )	return ;

    removeZTransform();
    if ( !zt )	return ;

    zaxistransform_ = zt;
    zaxistransform_->ref();

    if ( horsection_->geometry_ )
	updateZAxisVOI();
}


void HorizonSectionDataHandler::removeZTransform()
{
    if ( !zaxistransform_ )
	return;

    if ( zaxistransformvoi_!=-2 )
	zaxistransform_->removeVolumeOfInterest( zaxistransformvoi_ );

    zaxistransformvoi_ = -2;

    zaxistransform_->unRef();
    zaxistransform_ = 0;
}


void HorizonSectionDataHandler::generatePositionData( DataPointSet& dtpntset,
    double zshift, int sectionid ) const
{
    if ( !horsection_ || !horsection_->geometry_ ) return;

    if ( zaxistransform_ && zaxistransformvoi_>=0 )
    {
	if ( !zaxistransform_->loadDataIfMissing(zaxistransformvoi_,
				SilentTaskRunnerProvider()) )
	    return;
    }
    const char* hrsectionid = "Section ID";
    const DataColDef sidcol( hrsectionid );
    if ( dtpntset.dataSet().findColDef(sidcol,PosVecDataSet::NameExact)==-1 )
	dtpntset.dataSet().add( new DataColDef(sidcol) );

    const int sidcolidx =  dtpntset.dataSet().findColDef(
	sidcol, PosVecDataSet::NameExact ) - dtpntset.nrFixedCols();

    BinIDValueSet& bivs = dtpntset.bivSet();
    mAllocVarLenArr( float, vals, bivs.nrVals() );
    for ( int idx=0; idx<bivs.nrVals(); idx++ )
	vals[idx] = mUdf(float);

    vals[sidcolidx+dtpntset.nrFixedCols()] = sectionid;

    const int nrknots = horsection_->geometry_->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
    {
	const BinID bid = horsection_->geometry_->getKnotRowCol(idx);
	const StepInterval<int> displayrrg = horsection_->displayrrg_;
	const StepInterval<int> displaycrg = horsection_->displaycrg_;
	if ( horsection_->userchangedisplayrg_ &&
	    ( !displayrrg.includes(bid.inl(), false) ||
	      !displaycrg.includes(bid.crl(), false) ||
	    ((bid.inl()-displayrrg.start)%displayrrg.step) ||
	    ((bid.crl()-displaycrg.start)%displaycrg.step) ) )
	    continue;

	const Coord3 pos = horsection_->geometry_->getKnot(RowCol(bid),false);
	if ( !pos.isDefined() )
	    continue;

	float zval = pos.z_;
	if ( zshift )
	{
	    if ( !zaxistransform_ )
		zval += zshift;
	    else
	    {
		zval = zaxistransform_->transformTrc( bid, zval );
		if ( mIsUdf(zval) )
		    continue;

		zval += zshift;

		zval = zaxistransform_->transformTrcBack( bid, zval );
		if ( mIsUdf(zval) )
		    continue;
	    }
	}

	vals[0] = zval;
	bivs.add( bid, vals );
    }
}
