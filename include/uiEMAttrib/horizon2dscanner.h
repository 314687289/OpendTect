#ifndef horizon2dscanner_h
#define horizon2dscanner_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id: horizon2dscanner.h,v 1.5 2009-01-08 09:04:20 cvsranojay Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"
#include "posinfo.h"
#include "ranges.h"
#include "multiid.h"

class IOPar;
class BinIDValueSet;
namespace PosInfo { class Line2DData; }
namespace EM { class Horizon2DAscIO; }
namespace Table { class FormatDesc; }

mClass Horizon2DScanner : public Executor
{
public:

			Horizon2DScanner(const BufferStringSet& fnms,
					 const MultiID& setid,
					 Table::FormatDesc& fd);

    virtual const char*	message() const;
    virtual od_int64	totalNr() const;
    virtual od_int64	nrDone() const;
    virtual const char*	nrDoneText() const;

    bool		getLineNames(BufferStringSet&) const;
    BinIDValueSet*	getVals()			{ return bvalset_; }

    bool		reInitAscIO(const char*);
    void		setPosIsXY(bool yn)		{ isxy_ = yn; }
    bool		posIsXY() const			{ return isxy_; }

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(IOPar&) const;

protected:
    virtual int		nextStep();

    void		init();

    mutable int		totalnr_;
    EM::Horizon2DAscIO*	ascio_;
    BufferStringSet	filenames_;
    int			fileidx_;

    MultiID		setid_;
    BufferString	curline_;
    bool		isgeom_;
    bool		isxy_;
    TypeSet<Interval<float> > valranges_;
    Table::FormatDesc&	fd_;

    PosInfo::Line2DData	linegeom_;

    BufferStringSet	linenames_;
    BufferStringSet	validnms_;
    BufferStringSet	invalidnms_;
    BinIDValueSet*	bvalset_;
};


#endif
