#ifndef attribdescsettr_h
#define attribdescsettr_h

/*@+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: attribdescsettr.h,v 1.1 2005-02-03 15:35:02 kristofer Exp $
________________________________________________________________________

@$*/
 
#include <transl.h>
#include <ctxtioobj.h>
class Conn;
namespace Attrib { class DescSet; }

class AttribDescSetTranslatorGroup : public TranslatorGroup
{			  isTranslatorGroup(AttribDescSet)
public:
    			mDefEmptyTranslatorGroupConstructor(AttribDescSet)

    virtual const char*	defExtension() const		{ return "attr"; }
};


class AttribDescSetTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(AttribDescSet)

    virtual const char*	read(Attrib::DescSet&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual const char*	warningMsg() const		= 0;
    virtual const char*	write(const Attrib::DescSet&,Conn&) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(Attrib::DescSet&,const IOObj*,BufferString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
    static bool		store(const Attrib::DescSet&,const IOObj*,BufferString&);
			//!< BufferString has errmsg, if any
};



class dgbAttribDescSetTranslator : public AttribDescSetTranslator
{			     isTranslator(dgb,AttribDescSet)
public:
			mDefEmptyTranslatorConstructor(dgb,AttribDescSet)

    const char*		read(Attrib::DescSet&,Conn&);
    const char*		warningMsg() const		  { return warningmsg; }
    const char*		write(const Attrib::DescSet&,Conn&);

    BufferString	warningmsg;
};


#endif
