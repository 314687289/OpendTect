#ifndef uistratlayseqattrsetbuild_h
#define uistratlayseqattrsetbuild_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uistratlayseqattrsetbuild.h,v 1.3 2011-06-15 13:01:09 cvsbert Exp $
________________________________________________________________________

-*/

#include "uibuildlistfromlist.h"
#include "propertyref.h"
class CtxtIOObj;
namespace Strat { class RefTree; class LayerModel; class LaySeqAttribSet; }


/*!\brief allows user to define (or read) a set of layer sequence attributes */

mClass uiStratLaySeqAttribSetBuild : public uiBuildListFromList
{
public:
    			uiStratLaySeqAttribSetBuild(uiParent*,
						const Strat::LayerModel&);
    			~uiStratLaySeqAttribSetBuild();

    const Strat::LaySeqAttribSet& attribSet() const	{ return attrset_; }
    const PropertyRefSelection&	  propertyRefs() const	{ return props_; }

protected:

    Strat::LaySeqAttribSet&	attrset_;
    const Strat::RefTree&	reftree_;
    PropertyRefSelection	props_;
    CtxtIOObj&			ctio_;

    virtual void	editReq(bool);
    virtual void	removeReq();
    virtual bool	ioReq(bool);
    virtual const char*	avFromDef(const char*) const;

};


#endif
