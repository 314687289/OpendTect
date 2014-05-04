#ifndef uifont_h
#define uifont_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          22/05/2000
 RCS:           $Id$
________________________________________________________________________

-*/
#include "uibasemod.h"
#include "fontdata.h"
#include "uistring.h"

mFDQtclass(QFont)
mFDQtclass(QFontMetrics)
class Settings;
class uiParent;
class BufferStringSet;
class uiFont;

mGlobal(uiBase) bool select( uiFont&, uiParent* parnt=0,
			     const uiString& title=0 );
	/*!< \brief pops a selector box to select a new font
	     \return true if new font selected
	*/

mGlobal(uiBase) bool select( FontData&,uiParent* parnt=0,
			     const uiString& title = 0);


mExpClass(uiBase) uiFont : public CallBacker
{			//!< font stuff that needs Qt.

    friend bool 	select(uiFont&,uiParent*,const uiString&);
    friend class	uiFontList;

protected:
			uiFont(const char* ky, const char* family,
				int ps=FontData::defaultPointSize(),
				FontData::Weight w=FontData::defaultWeight(),
				bool it=FontData::defaultItalic());
			uiFont(const char* ky,FontData fd=FontData());
			uiFont(const uiFont&);

public:
			//! uiFont must be created through the uiFontList

    virtual		~uiFont();
    uiFont&		 operator=(const uiFont&);
    
    FontData		fontData() const ;
    void		setFontData(const FontData&); 
                        //!< Updates internal QFont and QFontMetrics.
    static void		setFontData(mQtclass(QFont)&,const FontData&);
    static void		getFontData(FontData&,const mQtclass(QFont)&);
    static mQtclass(QFont)* createQFont(const FontData&);
    
    inline const mQtclass(QFont&)	qFont() const { return *qfont_; }
    
    int			height() const;
    int			leading() const; 
    int 		maxWidth() const;
    int 		avgWidth() const;
    int 		width(const uiString&) const;
    int			ascent() const; 
    int			descent() const; 

    const char*		key() const		{ return key_; }
    Notifier<uiFont>    changed;    

protected: 

    // don't change order of these attributes!
    mQtclass(QFont*)		qfont_; 
    mQtclass(QFontMetrics&)	qfontmetrics_; 

    BufferString		key_;

    void			updateMetrics();

};


mExpClass(uiBase) uiFontList : public CallBacker
{
    friend class	uiFontSettingsGroup;

public:

			uiFontList() : inited_(false)	    {}
			~uiFontList();
    static uiFontList&	getInst();

    int			nrKeys();
    const char*		key(int);
    void		listKeys(BufferStringSet&);

    const ObjectSet<uiFont>&	fonts() const	{ return fonts_; }
    ObjectSet<uiFont>&	fonts()			{ return fonts_; }

    uiFont&		get(const char* ky=0);
    uiFont&		getFromQfnt(mQtclass(QFont*));

    uiFont&		add(const char* ky,const FontData&);
    uiFont&		add(const char* ky,
			    const char* f=FontData::defaultFamily(),
			    int ptsz=FontData::defaultPointSize(),
			    FontData::Weight w=FontData::defaultWeight(),
			    bool it=FontData::defaultItalic());

    void		use(const Settings&);
    void		update(Settings&);

protected:

    ObjectSet<uiFont>	fonts_;
    void		initialise();
    uiFont&		gtFont(const char*,const FontData* =0,
			       const mQtclass(QFont*) =0 );
private:

    bool		inited_;

    void		addOldGuess(const Settings&,const char*,int);
    void		removeOldEntries(Settings&);

};


#define FontList    uiFontList::getInst


#endif

