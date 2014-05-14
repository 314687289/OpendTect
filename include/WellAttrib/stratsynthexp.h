#ifndef stratsynthexp_h
#define stratsynthexp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		July 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "executor.h"

class IOObj;
class SeparString;
class SyntheticData;
class SeisTrcWriter;

namespace PosInfo { class Line2DData; }


mExpClass(WellAttrib) StratSynthExporter : public Executor
{
public:
    				StratSynthExporter(
				    const ObjectSet<const SyntheticData>& sds,
				    PosInfo::Line2DData* newgeom,
				    const SeparString&);
    				~StratSynthExporter();

    od_int64				nrDone() const;
    od_int64				totalNr() const;
    const char* 			nrDoneText() const
					{ return "Data Sets Created"; }
    const char* 			message() const;
protected:

    int 				nextStep();
    int					writePostStackTrace();
    int					writePreStackTraces();
    bool				prepareWriter();

    bool				isps_;
    const ObjectSet<const SyntheticData>& sds_;
    PosInfo::Line2DData*		linegeom_;
    SeisTrcWriter*			writer_;
    BufferString			prefixstr_;
    BufferString			postfixstr_;
    uiString				errmsg_;
    int					cursdidx_;
    int					posdone_;
};

#endif


