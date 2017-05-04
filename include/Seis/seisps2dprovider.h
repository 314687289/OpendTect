#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

*/

#include "seisprovider.h"
class SeisPS2DReader;


namespace Seis
{

class PS2DFetcher;

/*!\brief is the place to get traces from your 2D PS data stores.  */


mExpClass(Seis) PS2DProvider : public Provider2D
{ mODTextTranslationClass(Seis::PS2DProvider);
public:

			PS2DProvider();
			PS2DProvider(const DBKey&);
			~PS2DProvider();

    virtual GeomType	geomType() const	{ return LinePS; }

    virtual int		curLineIdx() const;
    virtual int		nrLines() const;
    virtual int		lineNr(Pos::GeomID) const;
    virtual BufferString lineName(int) const;
    virtual Pos::GeomID	geomID(int) const;
    virtual void	getGeometryInfo(int,PosInfo::Line2DData&) const;
    virtual bool	getRanges(int,StepInterval<int>&,
					  ZSampling&) const;

protected:

    friend class	PS2DFetcher;
    PS2DFetcher&	fetcher_;

    virtual void	doUsePar(const IOPar&,uiRetVal&);
    virtual void	doReset(uiRetVal&) const;
    virtual TrcKey	doGetCurPosition() const;
    virtual bool	doGoTo(const TrcKey&);
    virtual uiRetVal	doGetComponentInfo(BufferStringSet&,DataType&) const;
    virtual int		gtNrOffsets() const;
    virtual void	doGetNextGather(SeisTrcBuf&,uiRetVal&) const;
    virtual void	doGetGather(const TrcKey&,SeisTrcBuf&,uiRetVal&) const;
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const;

    SeisPS2DReader*	mkReader(Pos::GeomID) const;

};


} // namespace Seis
