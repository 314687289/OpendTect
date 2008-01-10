#ifndef uiattremout_h
#define uiattremout_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          January 2008
 RCS:           $Id: uiattremout.h,v 1.1 2008-01-10 08:41:18 cvshelene Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
#include "attribdescid.h"

class IOPar;
class MultiID;
class NLAModel;
class uiAttrSel;

namespace Attrib { class DescSet; }

/*! \brief
Brief Base class Earth Model Output Batch dialog.
Used for calculating attributes in relation with surfaces
*/


class uiAttrEMOut : public uiFullBatchDialog
{
public:
    			uiAttrEMOut(uiParent*,const Attrib::DescSet&,
				    const NLAModel*,const MultiID&,const char*);
			~uiAttrEMOut()			{};

protected:

    virtual void	attribSel(CallBacker*)		=0;
    virtual bool	prepareProcessing();
    virtual bool	fillPar(IOPar&);
    bool		addNLA(Attrib::DescID&);
    void		fillOutPar(IOPar&,const char* outtyp,
	    			   const char* idlbl,MultiID);

    Attrib::DescSet&	ads_;
    const MultiID&	nlaid_;
    Attrib::DescID	nladescid_;
    const NLAModel*	nlamodel_;

    uiAttrSel*		attrfld_;
};

#endif
