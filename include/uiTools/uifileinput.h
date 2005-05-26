#ifndef uifileinput_h
#define uifileinput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          21/9/2000
 RCS:           $Id: uifileinput.h,v 1.18 2005-05-26 15:43:09 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "uifiledlg.h"

class uiFileBrowser;
class uiPushButton;
class BufferStringSet;

/*! \brief A file-name input. 

Displays a uiLineEdit field showing the current selected file. The user can
edit the filename by hand, or pop up a file selector trough the included
"Select..." push button. Optional is an Examine button for browsing the file.

*/

class uiFileInput : public uiGenInput
{ 	
public:

    class Setup
    {
    public:
			Setup(const char* filenm=0)
			    : fnm(filenm)
			    , filter_("")
			    , forread_(true)
			    , withexamine_(false)
			    , dirs_(false)
			    {}
	BufferString	fnm;
	BufferString	filter_;
	bool		forread_;
	bool		withexamine_;
	bool		dirs_;
	
	#define mSetVar(var,val) var=val; return *this
	Setup&		filter(const char* s)	  { mSetVar(filter_,s); }
	Setup&		forread(bool yn=true)	  { mSetVar(forread_,yn); }
	Setup&		withexamine(bool yn=true) { mSetVar(withexamine_,yn); }
	Setup&		directories(bool yn=true) { mSetVar(dirs_,yn); }
        #undef mSetVar
    };

    			uiFileInput(uiParent*,const char* seltxt,
				    const char* fnm=0);
			uiFileInput(uiParent*,const char* seltxt,const Setup&);
			~uiFileInput();

    void		setFileName(const char*);
    void		setDefaultSelectionDir( const char* nm )
			{ defseldir = nm; }
    void		setFilter( const char* fltr )	{ filter = fltr; }
    const char*		selectedFilter() const		{ return selfltr; }
    void		setSelectedFilter(const char* fltr) { selfltr = fltr; }

    const char*		fileName();
    void		getFileNames(BufferStringSet&) const;

    uiFileDialog::Mode	selectMode() const		
			{ 
			    return selmodset ? selmode 
				 : (forread  ? uiFileDialog::ExistingFile 
					     : uiFileDialog::AnyFile); 
			}

    void		setSelectMode( uiFileDialog::Mode m) 
			{ selmodset = true;  selmode = m; }

    void		enableExamine(bool);
    			//!< only if examinebut present

protected:

    bool		forread;
    BufferString	fname;
    BufferString	filter;
    BufferString	defseldir;
    BufferString	selfltr;

    bool		selmodset;
    uiFileDialog::Mode  selmode;

    uiPushButton*	examinebut;

    virtual void	doSelect(CallBacker*);
    void		examineFile(CallBacker*);
    void		isFinalised(CallBacker*);
};


#endif
