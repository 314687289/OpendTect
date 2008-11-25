/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initattributes.cc,v 1.3 2008-11-25 15:35:22 cvsbert Exp $";

#include "initattributes.h"
#include "coherencyattrib.h"
#include "convolveattrib.h"
#include "dipfilterattrib.h"
#include "energyattrib.h"
#include "evaluateattrib.h"
#include "eventattrib.h"
#include "fingerprintattrib.h"
#include "frequencyattrib.h"
#include "freqfilterattrib.h"
#include "hilbertattrib.h"
#include "instantattrib.h"
#include "mathattrib.h"
#include "prestackattrib.h"
#include "positionattrib.h"
#include "referenceattrib.h"
#include "scalingattrib.h"
#include "shiftattrib.h"
#include "similarityattrib.h"
#include "specdecompattrib.h"
#include "volstatsattrib.h"

void Attributes::initStdClasses()
{
    Attrib::Coherency::initClass();
    Attrib::Convolve::initClass();
    Attrib::DipFilter::initClass();
    Attrib::Energy::initClass();
    Attrib::Evaluate::initClass();
    Attrib::Event::initClass();
    Attrib::FingerPrint::initClass();
    Attrib::Frequency::initClass();
    Attrib::FreqFilter::initClass();
    Attrib::Hilbert::initClass();
    Attrib::Instantaneous::initClass();
    Attrib::Math::initClass();
    Attrib::PreStack::initClass();
    Attrib::Position::initClass();
    Attrib::Reference::initClass();
    Attrib::Scaling::initClass();
    Attrib::Shift::initClass();
    Attrib::Similarity::initClass();
    Attrib::SpecDecomp::initClass();
    Attrib::VolStats::initClass();
}
