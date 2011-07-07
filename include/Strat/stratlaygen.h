#ifndef stratlaygen_h
#define stratlaygen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2011
 RCS:		$Id: stratlaygen.h,v 1.4 2011-07-07 14:47:20 cvsbert Exp $
________________________________________________________________________


-*/

#include "property.h"
class IOPar;

namespace Strat
{
class RefTree;
class LayerSequence;

/*!\brief Description that can generate layers and add these to a sequence.
 
  To be able to display this to users, youneed to be able to return a
  non-varying thickness. This dispThickness() can be the maximum possible
  (or if that is impossible, like 2 std devs), or an average/typical/center
  value.
 
 */

mClass LayerGenerator
{
public:	

    virtual const char*	name() const				= 0;
    virtual float	dispThickness(bool max=false) const	= 0;

    virtual bool	usePar(const IOPar&,const RefTree&);
    virtual void	fillPar(IOPar&) const;

    static LayerGenerator* get(const IOPar&,const RefTree&);
    mDefineFactoryInClass(LayerGenerator,factory);

    bool		generateMaterial(LayerSequence&,
			    Property::EvalOpts eo=Property::EvalOpts()) const;

    virtual bool	reset() const				{ return true; }
    virtual const char*	errMsg() const				{ return 0; }
    virtual void	syncProps(const PropertyRefSelection&)		= 0;
    virtual void	updateUsedProps(PropertyRefSelection&) const	= 0;

protected:

    virtual bool	genMaterial(LayerSequence&,
	    			    Property::EvalOpts) const	= 0;

};


#define mDefLayerGeneratorFns(clss,typstr) \
protected: \
    virtual bool	genMaterial(Strat::LayerSequence&, \
			    Property::EvalOpts eo=Property::EvalOpts()) const; \
public: \
    static const char*	typeStr()		{ return typstr; } \
    virtual const char* factoryKeyword() const	{ return typeStr(); } \
    static Strat::LayerGenerator* create()	{ return new clss; } \
    static void		initClass() { factory().addCreator(create,typeStr());} \
    virtual const char*	name() const; \
    virtual float	dispThickness(bool max=true) const; \
    virtual bool	usePar(const IOPar&,const Strat::RefTree&); \
    virtual void	fillPar(IOPar&) const; \
    virtual void	syncProps(const PropertyRefSelection&); \
    virtual void	updateUsedProps(PropertyRefSelection&) const



}; // namespace Strat

#endif
