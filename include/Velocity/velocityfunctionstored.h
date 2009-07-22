#ifndef velocityfunctionstored_h
#define velocityfunctionstored_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Umesh Sinha
Date:          Sep 2008
RCS:           $Id: velocityfunctionstored.h,v 1.3 2009-07-22 16:01:19 cvsbert Exp $
________________________________________________________________________


-*/

#include "velocityfunction.h"
#include "binidvalset.h"

class BinIDValueSet;
class MultiID;
class IOObjContext;

namespace Vel
{

class StoredFunctionSource;


/*!VelocityFunction that gets its information from a Velocity Picks. */

mClass StoredFunction : public Function
{
public:
				StoredFunction(StoredFunctionSource&);
    bool			moveTo(const BinID&);
    StepInterval<float> 	getAvailableZ() const;


protected:
    bool 			computeVelocity(float z0, float dz, int nr,
						float* res) const;

    bool			zit_;
    TypeSet<float>         	zval_;
    TypeSet<float>              vel_;
    TypeSet<float>              anisotropy_;

};


mClass StoredFunctionSource : public FunctionSource
{
public:
    static void                 initClass();
    				StoredFunctionSource();
    static const char*          sType() { return "StoredVelFunc"; }
    const char*                 type() const            { return sType(); }
    static IOObjContext&	ioContext();

    const VelocityDesc&         getDesc() const         { return desc_; }

    bool                        zIsTime() const;
    bool                        load(const MultiID&);
    bool                        store(const MultiID&);

    StoredFunction*            	createFunction(const BinID&);
   
    void			getAvailablePositions(BinIDValueSet&) const;
    bool			getVel(const BinID&,TypeSet<float>& zvals,
	                               TypeSet<float>& vel,
				       TypeSet<float>& anisotropy);

    void			setData(const BinIDValueSet&,
	    				const VelocityDesc&,bool zit);

    static const char*          sKeyZIsTime();
    static const char*          sKeyVelocityFunction();
    static const char*          sKeyHasAnisotropy();
    static const char*          sKeyVelocityType();

protected:
    void			fillIOObjPar(IOPar&) const;

    static FunctionSource* 	create(const MultiID&);
				~StoredFunctionSource();
    
    BinIDValueSet		veldata_;
    Threads::Mutex              readerlock_;
    bool                        zit_;
    VelocityDesc                desc_; 
    bool			hasanisotropy_;
};

}; // namespace


#endif
