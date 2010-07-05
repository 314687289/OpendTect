/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initseis.cc,v 1.8 2010-07-05 05:22:20 cvsnageswara Exp $";

#include "initseis.h"
#include "timedepthconv.h"
#include "seisseqio.h"
#include "segytr.h"
#include "seiscbvs.h"
#include "seis2dlineio.h"
#include "seispscubetr.h"

#define sKeySeisTrcTranslatorGroup "Seismic Data"
defineTranslatorGroup(SeisTrc,sKeySeisTrcTranslatorGroup);
defineTranslator(CBVS,SeisTrc,"CBVS");
defineTranslator(SEGY,SeisTrc,"SEG-Y");
defineTranslator(TwoD,SeisTrc,"2D");
defineTranslator(SeisPSCube,SeisTrc,"PS Cube");

mDefSimpleTranslatorSelector(SeisTrc,sKeySeisTrcTranslatorGroup)
mDefSimpleTranslatorioContext(SeisTrc,Seis)

void Seis::initStdClasses()
{
    LinearT2DTransform::initClass();
    LinearD2TTransform::initClass();
    Time2DepthStretcher::initClass();
    Depth2TimeStretcher::initClass();
    Seis::ODSeqInp::initClass();
    Seis::ODSeqOut::initClass();
}
