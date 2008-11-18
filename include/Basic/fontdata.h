#ifndef fontdata_h
#define fontdata_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: fontdata.h,v 1.10 2008-11-18 10:59:15 cvsbert Exp $
________________________________________________________________________

-*/

#include <enums.h>
#include <bufstring.h>


class FontData
{			//!< Data needed to make an actual font
public:    

    enum Weight		{ Light, Normal, DemiBold, Bold, Black };
			DeclareEnumUtils(Weight)
    static int		numWeight(Weight);
    static Weight	enumWeight(int);

			FontData( int ptsz=defaultpointsize,
				  const char* fam=defaultFamily(),
				  Weight wght=defaultweight,
				  bool ital=defaultitalic )
                        : family_(fam)
			, pointsize_(ptsz)
			, weight_(wght)
			, italic_(ital)		{}
			FontData( const char* s )
			: family_(defaultFamily())
			, pointsize_(defaultpointsize)
			, weight_(defaultweight)
			, italic_(defaultitalic)
			{ getFrom(s); }

			//! Store/retrieve (in FileMultiString format).
    void		getFrom(const char*);
    void		putTo(BufferString&);

    const char*		family() const		{ return family_; }
    int			pointSize() const	{ return pointsize_; }
    Weight		weight() const		{ return weight_; }
    bool		isItalic() const	{ return italic_; }

    void		setFamily( const char* f ) { family_ = f; }
    void		setPointSize( int ps )	{ pointsize_ = ps; }
    void		setWeight( Weight w )	{ weight_= w; }
    void		setItalic( bool yn )	{ italic_ = yn; }

    static const char*	defaultFamily();
    static int		defaultPointSize();
    static Weight	defaultWeight();
    static bool		defaultItalic();

    static void		setDefaultFamily( const char* f );
    static void		setDefaultPointSize( int ps );
    static void		setDefaultWeight( Weight w );
    static void		setDefaultItalic( bool yn );

    static const char* const* universalFamilies();
			//!< array of strings with null as last element

    static const char* const* defaultKeys();
			//!< array of strings with null as last element

    enum StdSz		{ Control=0, 
			  GraphicsMed, GraphicsSmall, GraphicsLarge,
			  ControlSmall, ControlLarge, Fixed };
    static const char* key( StdSz ss );
 
protected:
 
    BufferString	family_;
    int			pointsize_;
    Weight		weight_;
    bool		italic_;

    static BufferString	defaultfamily;
    static int		defaultpointsize;
    static Weight	defaultweight;
    static bool		defaultitalic;
    static const char*	universalfamilies[];
    static const char*	defaultkeys[];

};


#endif
