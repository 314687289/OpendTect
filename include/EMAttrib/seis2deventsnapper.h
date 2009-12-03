#ifndef seis2deventsnapper_h
#define seis2deventsnapper_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          October 2009
 RCS:           $Id: seis2deventsnapper.h,v 1.2 2009-12-03 06:57:29 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "seiseventsnapper.h"
#include "seistrc.h"
#include "seisread.h"
#include "executor.h"

namespace EM { class Hor2DSeisLineIterator; class Horizon2D; }
namespace Seis { class Horizon2D; }
class SeisTrcReader;
class IOObj;


mClass Seis2DEventSnapper : public SeisEventSnapper
{
public:

    mClass Setup
    {
    public:
				Setup(const IOObj* seisobj,const LineKey& l,
				      const Interval<float>& gt)
				    : ioobj_(seisobj)
				    , lk_(l)
				    , gate_(gt)	{}
	mDefSetupMemb(const IOObj*,ioobj)
	mDefSetupMemb(LineKey,lk)
	mDefSetupMemb(Interval<float>,gate)
    };
				Seis2DEventSnapper(const EM::Horizon2D&,
					   EM::Horizon2D&,
					   const Seis2DEventSnapper::Setup&);
				~Seis2DEventSnapper();

protected:
    virtual int			nextStep();

    int				horlineidx_;
    SeisTrc			trc_;
    SeisTrcReader*		seisrdr_;
    const EM::Horizon2D&	orghor_;
    EM::Horizon2D&		newhor_;
};


mClass Seis2DLineSetEventSnapper : public ExecutorGroup
{
public:

    mClass Setup
    {
    public:
				Setup(const BufferString& atrnm,int typ,
				      const Interval<float>& gt)
				    : attrnm_(atrnm)
				    , type_(typ)
				    , gate_(gt) {}
	mDefSetupMemb(BufferString,attrnm)
	mDefSetupMemb(int,type)
	mDefSetupMemb(Interval<float>,gate)
    };
				Seis2DLineSetEventSnapper(const EM::Horizon2D*,
					EM::Horizon2D*,const Setup&);
				~Seis2DLineSetEventSnapper();
protected:
    int				type_;
    BufferString		attribnm_;
    Interval<float>		gate_;
    const EM::Horizon2D*	orghor_;
    EM::Horizon2D*		newhor_;
    EM::Hor2DSeisLineIterator*	hor2diterator_;
};

#endif
