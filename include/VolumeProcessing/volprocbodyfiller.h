#ifndef volprocbodyfiller_h
#define volprocbodyfiller_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		November 2007
 RCS:		$Id: volprocbodyfiller.h,v 1.3 2010-02-24 22:28:56 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "volprocchain.h"
#include "arrayndimpl.h"

namespace EM { class EMObject; class Body; class ImplicitBody; }

namespace VolProc
{

class Step;

mClass BodyFiller : public Step
{
public:
    static void			initClass();
    				BodyFiller(Chain&);
    				~BodyFiller();

    const char*			type() const { return sKeyType(); }
    bool			needsInput(const HorSampling&) const; 
    bool			areSamplesIndependent() const { return true; }
    
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    void			setOutput(Attrib::DataCubes*);

    float			getInsideValue()  { return insideval_; }
    float			getOutsideValue() { return outsideval_; } 
    void			setInsideOutsideValue(const float inside, 
	    					      const float ouside);
    
    bool			setSurface(const MultiID&);
    MultiID			getSurfaceID() { return mid_; }
    Task*			createTask();

    static const char*		sKeyType() { return "BodyFiller"; }
    static const char*		sKeyOldType() { return "MarchingCubes"; }
    static const char* 		sUserName(){ return "Body shape painter"; }
    static const char*		sKeyMultiID(){return "Body ID"; }
    static const char*		sKeyOldMultiID(){return "MarchingCubeSurface ID"; }
    static const char*		sKeyInsideOutsideValue() 
    					{ return "Surface InsideOutsideValue"; }
   
protected:

    bool			prefersBinIDWise() const	{ return true; }
    bool			prepareComp(int nrthreads)	{ return true; }
    bool			computeBinID(const BinID&,int); 
    bool			getFlatPlgZRange(const BinID&,
						 Interval<double>& result); 
    static Step*		create(Chain&);

    EM::Body*			body_;
    EM::EMObject*		emobj_;
    EM::ImplicitBody*		implicitbody_;
    MultiID			mid_;

    float			insideval_;
    float			outsideval_;

    				//For flat body_ only, no implicitbody_.
    CubeSampling		flatpolygon_;
    TypeSet<Coord3>		plgknots_;
    TypeSet<Coord3>		plgbids_;
    char			plgdir_;
    				/* inline=0; crosline=1; z=2; other=3 */
    double			epsilon_;
};


}; //namespace

#endif
