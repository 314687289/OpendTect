#ifndef attribposvecoutput_h
#define attribposvecoutput_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          June 2005
 RCS:           $Id: attribposvecoutput.h,v 1.7 2012-08-03 13:00:08 cvskris Exp $
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "executor.h"
#include "bufstringset.h"

class NLAModel;
class BinIDValueSet;
class PosVecDataSet;

namespace Attrib
{

class EngineMan;
class DescSet;

mClass(AttributeEngine) PosVecOutputGen : public Executor
{
public:

		PosVecOutputGen(const DescSet&,
				const BufferStringSet& attribdefkeys,
				const ObjectSet<BinIDValueSet>& positions,
				ObjectSet<PosVecDataSet>& output,
				const NLAModel* mdl=0);
		~PosVecOutputGen();

    const char*	message() const;
    const char*	nrDoneText() const
		{ return outex_ ? outex_->nrDoneText() : "Positions handled"; }
    od_int64	nrDone() const
		{ return outex_ ? outex_->nrDone() : 0; }
    od_int64	totalNr() const
		{ return outex_ ? outex_->totalNr() : -1; }

protected:

    const BufferStringSet&	inps_;
    const DescSet&		ads_;
    const ObjectSet<BinIDValueSet>& bvss_;
    ObjectSet<PosVecDataSet>&	vdss_;
    EngineMan*			aem_;
    Executor*			outex_;
    mutable BufferString	msg_;

    int				nextStep();
};

}; // namespace Attrib

#endif

