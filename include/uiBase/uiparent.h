#ifndef uiparent_h
#define uiparent_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2001
 RCS:           $Id: uiparent.h,v 1.16 2007-09-28 03:50:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uilayout.h"

class uiObjHandle;
class uiObjectBody;
class uiObject;
class uiMainWin;
class Color;
class uiParentBody;
class uiCursor;


class uiParent : public uiObjHandle
{
friend class uiParentBody;
friend class uiObjectBody;
public:
			uiParent( const char* nm, uiParentBody* );

    void		addChild( uiObjHandle& );
    void		manageChld( uiObjHandle&, uiObjectBody& );
    void                attachChild ( constraintType tp, uiObject* child,
				      uiObject* other, int margin,
				      bool reciprocal );

    const ObjectSet<uiObjHandle>* childList() const;

			//! persists current widget position
    void		storePosition();
			//! restores persisted widget position
    void		restorePosition();

    virtual uiMainWin*	mainwin()		{ return 0; }

    uiObject*		mainObject()			{ return mainobject(); }
    const uiObject*	mainObject() const
			    { return const_cast<uiParent*>(this)->mainobject();}

    uiParentBody*	pbody();
    const uiParentBody*	pbody() const
			{ return const_cast<uiParent*>(this)->pbody(); }


#define mIfMO()		if ( mainObject() ) mainObject()
#define mRetMO(fn,val)	return mainObject() ? mainObject()->fn() : val;

    void                attach( constraintType t, int margin=-1 )
                            { mIfMO()->attach(t,margin); }
    void                attach( constraintType t, uiParent* oth, int margin=-1,
                                bool reciprocal=true)
                            { attach(t,oth->mainObject(),margin,reciprocal); }
    void		attach( constraintType t, uiObject* oth, int margin=-1,
                                bool reciprocal=true)
                            { attach_(t,oth,margin,reciprocal); }


    virtual void	display( bool yn = true, bool shrk=false,
				 bool maximize=false )
			    { finalise(); mIfMO()->display(yn,shrk,maximize); }

    void		setFocus()                { mIfMO()->setFocus(); }
    bool		hasFocus() const	  { mRetMO(hasFocus,false); }

    void		setSensitive(bool yn=true){ mIfMO()->setSensitive(yn); }
    bool		sensitive() const	  { mRetMO(sensitive,false); }

    const uiFont*	font() const		  { mRetMO(font,0); }
    void		setFont( const uiFont& f) { mIfMO()->setFont(f); }
    void		setCaption(const char* c) { mIfMO()->setCaption(c); }
    void		setCursor(const uiCursor& c) { mIfMO()->setCursor(c);}

    uiSize		actualsize( bool include_border) const
			{
			    if ( mainObject() ) 
				return mainObject()->actualsize(include_border);
			    return uiSize();
			}

    int			prefHNrPics() const	  { mRetMO(prefHNrPics, -1 ); }
    int			prefVNrPics() const	  { mRetMO(prefVNrPics,-1); }
    void		setPrefHeight( int h )    { mIfMO()->setPrefHeight(h); }
    void		setPrefWidth( int w )     { mIfMO()->setPrefWidth(w); }
    void		setPrefWidthInChar( float w )		 
			    { mIfMO()->setPrefWidthInChar(w); }
    void		setPrefHeightInChar( float h )
			    { mIfMO()->setPrefHeightInChar(h); }

    virtual void	reDraw( bool deep ) 	  { mIfMO()->reDraw( deep ); }
    void		shallowRedraw( CallBacker* =0 )         {reDraw(false);}
    void		deepRedraw( CallBacker* =0 )            {reDraw(true); }

    void		setStretch( int h, int v ){ mIfMO()->setStretch(h,v); }

    const Color&	backgroundColor() const;
    void		setBackgroundColor(const Color& c)
			    { mIfMO()->setBackgroundColor(c); }

protected:

    virtual void        attach_( constraintType t, uiObject* oth, int margin=-1,
                                bool reciprocal=true)
                            { mIfMO()->attach(t,oth,margin,reciprocal); }

#undef mIfMO
#undef mRetMO

    virtual uiObject*	mainobject()	{ return 0; }
};

#endif
