/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/
 
static const char* rcsID = "$Id: linear.cc,v 1.6 2005-08-19 13:48:01 cvskris Exp $";


#include "linear.h"
#include "undefval.h"
#include <math.h>

LinePars* SecondOrderPoly::createDerivative() const
{
    return new LinePars( b, a*2 );
}

#define mArrVal(arr) (*(arr + idx * offs))

static void calcLS( LinStats2D& ls, const float* xvals, const float* yvals,
		    int nrpts, int offs )
{
    double sumx = 0, sumy = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	sumx += mArrVal(xvals);
	sumy += mArrVal(yvals);
    }

    ls.lp.a0 = (float)sumx; ls.lp.ax = 0;
    ls.sd.a0 = 0; Values::setUdf(ls.sd.ax);
    ls.corrcoeff = 1;
    if ( nrpts < 2 )
	return;

    float avgx = sumx / nrpts;
    float avgy = sumy / nrpts;
    double sumxy = 0, sumx2 = 0, sumy2 = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	float xv = mArrVal(xvals) - avgx;
	float yv = mArrVal(yvals) - avgy;
	sumx2 += xv * xv;
	sumy2 += yv * yv;
	sumxy += xv * yv;
    }

    if ( sumx2 < 1e-30 )
    {
	// No x range
	ls.lp.a0 = 0;
	Values::setUdf(ls.lp.ax);
	Values::setUdf(ls.sd.a0); ls.sd.ax = 0;
	ls.corrcoeff = 1;
	return;
    }
    else if ( sumy2 < 1e-30 )
    {
	// No y range
	ls.lp.a0 = avgy;
	ls.lp.ax = 0;
	ls.corrcoeff = 1;
	ls.sd.a0 = ls.sd.ax = 0;
	return;
    }

    ls.lp.ax = sumxy / sumx2;
    ls.lp.a0 = (sumy - sumx*ls.lp.ax) / nrpts;
    ls.corrcoeff = sumxy / (sqrt( sumx2 ) * sqrt( sumy2 ));

    double sumd2 = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	const float ypred = ls.lp.a0 + ls.lp.ax * mArrVal(xvals);
	const float yd = mArrVal(yvals) - ypred;
	sumd2 = yd * yd;
    }
    if ( nrpts < 3 )
	ls.sd.ax = ls.sd.a0 = 0;
    else
    {
	ls.sd.ax = sqrt( sumd2 / ((nrpts-2) * sumx2) );
	ls.sd.a0 = sqrt( (sumx2 * sumd2) / (nrpts * (nrpts-2) * sumx2) );
    }
}


void LinStats2D::use( const float* xvals, const float* yvals, int nrpts )
{
    calcLS( *this, xvals, yvals, nrpts, 1 );
}


void LinStats2D::use( const Geom::Point2D<float>* vals, int nrpts )
{
    if ( nrpts < 1 ) return;
    calcLS( *this, &vals[0].x(), &vals[0].y(), nrpts, 2 );
}


void AxisLayout::setDataRange( Interval<float> intv )
{
    sd.start = intv.start;
    bool rev = intv.start > intv.stop;
    if ( rev ) Swap( intv.start, intv.stop );
    float wdth = intv.width();

    // guard against zero interval
    float indic = intv.start + wdth;
    float indic_start = intv.start;
    if ( mIsZero(indic,mDefEps) ) { indic_start += 1; indic += 1; }
    indic = 1 - indic_start / indic;
    if ( mIsZero(indic,mDefEps) )
    {
	sd.start = intv.start - 1;
	sd.step = 1;
	stop = intv.start + 1;
	return;
    }

    double scwdth = wdth < 1e-30 ? -30 : log10( wdth );
    int tenpow = 1 - (int)scwdth; if ( scwdth < 0 ) tenpow++;
    double stepfac = IntPowerOf( 10, tenpow );
    scwdth = wdth * stepfac;

    double scstep;
    if ( scwdth < 15 )          scstep = 2.5;
    else if ( scwdth < 30 )     scstep = 5;
    else if ( scwdth < 50 )     scstep = 10;
    else                        scstep = 20;

    sd.step = scstep / stepfac;
    if ( wdth > 1e-30 )
    {
	float fidx = rev ? ceil( intv.stop / sd.step + 1e-6 )
			 : floor( intv.start / sd.step + 1e-6 );
	sd.start = mNINT( fidx ) * sd.step;
    }
    if ( rev ) sd.step = -sd.step;

    stop = findEnd( intv.stop );
}


float AxisLayout::findEnd( float datastop ) const
{
    SamplingData<float> worksd = sd;
    bool rev = datastop < worksd.start;
    if ( rev ) Swap( datastop, worksd.start );
    if ( worksd.step < 0 ) worksd.step = -worksd.step;

    if ( worksd.start + 10000 * worksd.step < datastop )
	return datastop;

    float pos = ceil( (datastop-sd.start) / worksd.step - 1e-6 );
    if ( pos < .5 ) pos = 1;
    float wdth = mNINT(pos) * worksd.step;

    return sd.start + (rev ? -wdth : wdth);
}
