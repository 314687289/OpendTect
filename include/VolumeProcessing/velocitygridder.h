#ifndef velocitygridder_h
#define velocitygridder_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
 RCS:		$Id: velocitygridder.h,v 1.9 2010-07-12 22:52:41 cvskris Exp $
________________________________________________________________________


-*/

#include "volprocchain.h"
#include "veldesc.h"

class Gridder2D;
namespace Vel
{
    class Function;
    class FunctionSource;
    class GriddedFunction;
    class GriddedSource;
}


namespace VolProc
{

mClass VelGriddingStep : public VolProc::Step
{
public:
    static void		initClass();

			VelGriddingStep(VolProc::Chain&);
    			~VelGriddingStep();

    const char*		type() const			{ return sType(); }
    const VelocityDesc* getVelDesc() const;

    void		setSources(ObjectSet<Vel::FunctionSource>&);
    const ObjectSet<Vel::FunctionSource>&	getSources() const;

    void		setGridder(Gridder2D*); //becomes mine
    const Gridder2D*	getGridder() const;

    bool		needsInput(const HorSampling&) const;
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    void		releaseData();
    bool		canInputAndOutputBeSame() const	{ return true; }
    bool		needsFullVolume() const		{ return true;}
    
    const char*		errMsg() const		{ return errmsg_.str(); }

    static const char*	sType()			{ return "Gridding"; }
    static const char*	sUserName()		{ return sType(); }
    static const char*	sKeyType()		{ return "Type"; }
    static const char*	sKeyID()		{ return "ID"; }
    static const char*	sKeyNrSources()		{ return "NrSources"; }
    static const char*	sKeyGridder()		{ return "Gridder"; }

protected:

    Task*				createTask();

    int					addFunction(const BinID&,int);
    void				removeOldFunctions();
    static VolProc::Step*		create(VolProc::Chain&);

    Gridder2D*				gridder_;
    ObjectSet<Vel::FunctionSource>	sources_;
    BufferString			errmsg_;
};


}; //namespace

#endif
