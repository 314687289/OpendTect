/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: initvolumeprocessing.cc 37082 2014-10-29 16:52:02Z nanne.hemstra@dgbes.com $";


#include "moddepmgr.h"
#include "velocitygridder.h"
#include "volprocattrib.h"
#include "volprocbodyfiller.h"
#include "volprochorinterfiller.h"
#include "volproclateralsmoother.h"
#include "volprocregionfiller.h"
#include "volprocsmoother.h"
#include "volprocsurfacelimitedfiller.h"
#include "volproctrans.h"
#include "volprocvolreader.h"
#include "wellloginterpolator.h"


mDefModInitFn(VolumeProcessing)
{
    mIfNotFirstTime( return );

    VolProcessingTranslatorGroup::initClass();
    dgbVolProcessingTranslator::initClass();

    VolProc::VolumeReader::initClass();
    VolProc::BodyFiller::initClass();
    VolProc::HorInterFiller::initClass();
    VolProc::SurfaceLimitedFiller::initClass();
    VolProc::RegionFiller::initClass();
    VolProc::LateralSmoother::initClass();
    VolProc::Smoother::initClass();
    VolProc::ExternalAttribCalculator::initClass();
    VolProc::VelocityGridder::initClass();
    VolProc::WellLogInterpolator::initClass();

#ifdef __debug__
    VolProcAttrib::initClass();
#endif
}
