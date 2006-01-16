#ifndef uiiosel_h
#define uiiosel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiiosel.h,v 1.29 2006-01-16 12:20:40 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "bufstringset.h"
class UserIDSet;
class uiLabeledComboBox;
class uiPushButton;
class IOPar;


/*! \brief UI element for selection of data objects */

class uiIOSelect : public uiGroup
{
public:

			uiIOSelect(uiParent*,const CallBack& do_selection,
				   const char* txt,
				   bool withclear=false,
				   const char* buttontxt="Select ...",
				   bool keepmytxt=false);
			~uiIOSelect();

    const char*		getInput() const;
    const char*		getKey() const;
    void		setInput(const char* key);
			//!< Will fetch user name using userNameFromKey
    void		setInputText(const char*);
    			//!< As if user typed it manually

    int			nrItems() const;
    int			getCurrentItem() const;
    void		setCurrentItem(int);
    const char*		getItem(int) const;

    void		addSpecialItem(const char* key,const char* value=0);
			//!< If value is null, add value same as key

    virtual void	updateHistory(IOPar&) const;
    virtual void	getHistory(const IOPar&);

    void		clear()			{ setCurrentItem( 0 ); }
    void		empty(bool withclear=false);
    virtual void	processInput()		{}
    void		setReadOnly(bool readonly=true);

    void		doSel(CallBacker*);
    			//!< Called by 'Select ...' button push.
    			//!< Make sure selok_ is true if that is the case!
    Notifier<uiIOSelect> selectiondone;

    const char*		labelText() const;
    void		setLabelText(const char*);

    void		stretchHor(bool);

protected:

    CallBack		doselcb_;
    BufferStringSet	entries_;
    IOPar&		specialitems;
    bool		selok_;
    bool		keepmytxt_;

    uiLabeledComboBox*	inp_;
    uiPushButton*	selbut_;

    void		selDone(CallBacker*);
			//!< Subclass must call it - base class can't
			//!< determine whether a selection was successful.

    virtual const char*	userNameFromKey( const char* s ) const	{ return s; }
			//!< If 0 returned, then if possible,
			//!< that entry is not entered in the entries_.

    void		checkState() const;
    void		updateFromEntries();
    bool		haveEntry(const char*) const;

    virtual void	objSel()		{}
			//!< notification when user selects from combo

    void		doFinalise();

};


class uiIOFileSelect : public uiIOSelect
{
public:
			uiIOFileSelect(uiParent*,const char* txt,
					bool for_read,
					const char* inp=0,
					bool withclear=false);

    void		setFilter( const char* f )	{ filter = f; }
    void		selectDirectory( bool yn=true )	{ seldir = yn; }

    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

			// Some standard types of files
    static IOPar&	ixtablehistory;
    static IOPar&	devicehistory;
    static IOPar&	tmpstoragehistory;

protected:

    void		doFileSel(CallBacker*);
    bool		forread;
    BufferString	filter;
    bool		seldir;

};


/*!\mainpage User Interface Tools

  The uiTools module is a collection of tools that are based on the uiBase
  module (and thus on Qt services) but that combine uiBase services without
  accessing Qt at all.

  Some commonly used classes:
<ul>
 <li>uiGenInput is the most common user interface element we use. It represents an generalise input field for strings, numbers, bools, positions and more. The idea is to tell the class the characteristics of the data you need to input. uiGenInput will then select the best user interfac element for that type of data.
 <li>Subselection of Inlines/Crosslines and Z can be done with uiBinIDSubSel .
 <li>ColorTableEditor displays a color table with scaling and editing
 <li>uiExecutor executes the work encapsulated in an Executor object. An Executor object holds an amount of work that is probably too big to be finished instantly, hence the user must be able to see the progress and must be able to stop it.
 <li>uiFileBrowser shows the contents of text files. If editable, the user can edit the text and save it.
</ul>

*/


#endif
