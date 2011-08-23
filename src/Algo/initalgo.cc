/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initalgo.cc,v 1.21 2011-08-23 06:54:11 cvsbert Exp $";

#include "initalgo.h"

#include "gridder2d.h"
#include "array2dinterpolimpl.h"
#include "posfilterstd.h"
#include "windowfunction.h"
#include "fourier.h"

void Algo::initStdClasses()
{
    mIfNotFirstTime( return );

    BartlettWindow::initClass();
    BoxWindow::initClass();
    CosTaperWindow::initClass();
    HammingWindow::initClass();
    HanningWindow::initClass();

    InverseDistanceGridder2D::initClass();
//TriangulatedNeighborhoodGridder2D::initClass();
//Not good enough for production
    TriangulatedGridder2D::initClass();

    Pos::RandomFilter3D::initClass();
    Pos::RandomFilter2D::initClass();
    Pos::SubsampFilter3D::initClass();
    Pos::SubsampFilter2D::initClass();

    InverseDistanceArray2DInterpol::initClass();
    TriangulationArray2DInterpol::initClass();
    Array2DInterpolExtension::initClass();

    Fourier::CC::initClass();
}
