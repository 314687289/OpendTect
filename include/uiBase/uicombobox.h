#ifndef uiComboBox_H
#define uiComboBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.h,v 1.13 2003-04-01 10:08:08 arend Exp $
________________________________________________________________________

-*/
#include <uigroup.h>
#include <userinputobj.h>

class PtrUserIDObjectSet;
class uiLabel;
class uiComboBoxBody;

class BufferString;
template <class T> class ObjectSet;

class uiComboBox : public uiObject, public UserInputObjImpl<int>
{
public:

			uiComboBox(uiParent*,const char* nm="Combo Box",
				   bool editable=false);
			uiComboBox(uiParent*,const PtrUserIDObjectSet&,
				   bool editable=false);
    virtual 		~uiComboBox();

			/*!  This is the text that is actually in the current
			     item. This text may differ from
			     textOfItem(currentItem()) when the box is editable.
			*/
    const char*		text() const;
    void		setText(const char*);

    bool		isPresent(const char*) const;

    void		empty();
    int			size() const;


    virtual bool	hasItems()		{ return true; }
    virtual void	addItem(const char*); 
/*
    void		addItems(const char**); 
    void		addItems(const PtrUserIDObjectSet&);
    void		addItems(const ObjectSet<BufferString>&);
*/
    int			currentItem() const;
    void		setCurrentItem(int);
    void		setCurrentItem(const char*); //!< First match
    const char*		textOfItem(int) const;
    void		setItemText(int,const char*);

			//! Triggered when selection has changed.
    Notifier<uiComboBox> selectionChanged;

    virtual void        setReadOnly( bool = true );
    virtual bool        isReadOnly() const;

    virtual bool	update( const DataInpSpec& spec );

protected:

    virtual void        setvalue_( int i )	{ setCurrentItem(i); }
    virtual int		getvalue_() const	{ return currentItem(); }
    virtual void	clear_()		{ setCurrentItem(0); }

    virtual bool	notifyValueChanging_( const CallBack& )	{ return false;}
    virtual bool	notifyValueChanged_( const CallBack& cb )   
			    { selectionChanged.notify(cb); return true; }

private:

    BufferString	rettxt;

    uiComboBoxBody*	body_;
    uiComboBoxBody&	mkbody(uiParent*, const char*, bool);

};


class uiLabeledComboBox : public uiGroup
{
public:
		uiLabeledComboBox( uiParent*,const char* txt,
				   const char* nm="Labeled Combobox",
				   bool editable=false);
		uiLabeledComboBox( uiParent*,const PtrUserIDObjectSet&,
					    bool ed=false);

    uiComboBox*	box()		{ return cb; }
    uiLabel*	label()		{ return labl; }


protected:

    uiComboBox*	cb;
    uiLabel*	labl;

};

#endif
