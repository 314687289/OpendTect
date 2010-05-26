#ifndef createattriblogdlg_h
#define createattriblogdlg_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2008
 RCS:           $Id: createattriblog.h,v 1.1 2010-05-26 09:26:26 cvsbruno Exp $
 _______________________________________________________________________

-*/

#include "binidvalset.h"

namespace Attrib { class DescSet; class SelSpec; class EngineMan; }
namespace Well { class Data; }
class NLAModel;
class TaskRunner;

mClass AttribLogExtractor
{
public:
				AttribLogExtractor(const Well::Data& wd)
				    : wd_(&wd)
				    , bidset_(BinIDValueSet(2,true))  
				    {}

    const TypeSet<BinIDValueSet::Pos>& positions() const { return  positions_; }
    const TypeSet<float>& 	depths() const  	{ return depths_; }
    const BinIDValueSet& 	bidset() const		{ return bidset_; } 

    bool                        extractData(Attrib::EngineMan&,TaskRunner* t=0);
    bool                        fillPositions(const StepInterval<float>&);
    void			setWD(const Well::Data& wd)
				{ wd_ = &wd; }

protected:

    const Well::Data*		wd_;
    TypeSet<BinIDValueSet::Pos> positions_;
    BinIDValueSet 		bidset_; 
    TypeSet<float>		depths_;
};


mClass AttribLogCreator
{
public:

    mClass Setup
    {
    public:
				Setup(const Attrib::DescSet* attr)
				    : nlamodel_(0)
				    , attrib_(attr)
				    , tr_(0)
				    , extractstep_(0.15)	    
				    , topmrknm_(0)     	
				    , botmrknm_(0)     	
				    {}

	mDefSetupMemb(const NLAModel*,nlamodel)
	mDefSetupMemb(const Attrib::DescSet*,attrib)
	mDefSetupMemb(Attrib::SelSpec*,selspec)
	mDefSetupMemb(float,extractstep)
	mDefSetupMemb(const char*,topmrknm)
	mDefSetupMemb(const char*,botmrknm)
	mDefSetupMemb(BufferString,lognm)
	mDefSetupMemb(TaskRunner*,tr) //optional
    };
    

				AttribLogCreator(const Setup& su, int& selidx)
				    : setup_(su)
				    , extractor_(0)
				    , sellogidx_(selidx)	   
				    {}
				~AttribLogCreator() {}

    bool 			doWork(Well::Data&,BufferString&);

protected:

    const Setup&		setup_;
    AttribLogExtractor*		extractor_;
    int&			sellogidx_;

    bool                        extractData(BinIDValueSet&);
    void                        setUpRange(const Well::Data&,Interval<float>&);
    bool                        createLog(Well::Data&,const AttribLogExtractor&);

};



#endif
