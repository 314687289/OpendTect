#ifndef uisteeringsel_h
#define uisteeringsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uisteeringsel.h,v 1.16 2010-09-30 15:14:00 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "uiattrsel.h"
#include "attribdescid.h"

class uiGenInput;
class uiLabel;
namespace Attrib { class Desc; class DescSet; class SelSpec; };


mClass uiSteerCubeSel : public uiSeisSel
{
public:

				uiSteerCubeSel(uiParent*,CtxtIOObj&,
				       const Attrib::DescSet*,bool,
				       const char* txt="Steering Data" );

    inline Attrib::DescID	inlDipID() const	{ return getDipID(0); }
				// Returns -2 when selected is not a dip
    inline Attrib::DescID 	crlDipID() const	{ return getDipID(1); }
				// Returns -2 when selected is not a dip

    void			setDesc(const Attrib::Desc*);
    void			setDescSet(const Attrib::DescSet*);

    void			fillSelSpec(Attrib::SelSpec&,bool inl);
    				/* inl=true: AttribSelSpec for inline comp
    				   inl=false: AttribSelSpec for crossline comp*/

    static const IOObjContext&	ioContext();
    static CtxtIOObj*		mkCtxtIOObj(bool is2d,bool forread);

protected:

    Attrib::DescID		getDipID(int) const;
    void			doFinalise(CallBacker*);

    uiAttrSelData		attrdata_;
};



/*!\brief Attribute Steering ui element: data + selection of type. */

mClass uiSteeringSel : public uiGroup
{
public:
				uiSteeringSel(uiParent*,
					      const Attrib::DescSet*,bool is2d,
					      bool withconstdir=true,
					      bool doinit=true);
				~uiSteeringSel();

    Attrib::DescID		descID();

    virtual bool		willSteer() const;
    void			setDesc(const Attrib::Desc*);
    void			setDescSet(const Attrib::DescSet*);
    void			setType(int,bool fixed=false);

    static IOPar&		inpselhist;

protected:

    const Attrib::DescSet*	descset_;
    CtxtIOObj&			ctio_;

    uiLabel*			nosteerlbl_;
    uiGenInput*			typfld_;
    uiGenInput*			dirfld_;
    uiGenInput*			dipfld_;
    uiSteerCubeSel*		inpfld_;

    bool			is2d_;
    bool			notypechange_;
    bool			withconstdir_;

    void			createFields();
    void			doFinalise(CallBacker*);
    virtual void		typeSel(CallBacker*);
};

#endif
