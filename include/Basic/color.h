#ifndef color_h
#define color_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		22-3-2000
 RCS:		$Id: color.h,v 1.17 2008-12-23 14:44:49 cvsbert Exp $
________________________________________________________________________

Color is an RGB color object, with a transparancy. The storage is in a 4-byte
integer, similar to Qt.

-*/


#include "gendefs.h"


mClass Color
{
public:

			Color( unsigned char r_=255, unsigned char g_=255,
				unsigned char b_=255, unsigned char t_=0 );
			Color( unsigned int rgbval );

    bool		operator ==( const Color& c ) const;
    bool		operator !=( const Color& c ) const;

    unsigned char	r() const;
    unsigned char	g() const;
    unsigned char	b() const;
    unsigned char	t() const;

    bool		isVisible() const;

    unsigned int	rgb() const;
    unsigned int&	rgb();

    void         	set( unsigned char r_, unsigned char g_,
			     unsigned char b_, unsigned char t_=0 );

    Color		complementaryColor() const;
    Color		operator*(float) const;
    void		lighter( float f=1.1 );
    void        	setRgb( unsigned int rgb_  );
    void		setTransparency( unsigned char t_ );
    void		setHSV(unsigned char h,unsigned char s,unsigned char v);
    void		getHSV(unsigned char&,unsigned char&,unsigned char&);
    void		setStdStr(const char*); //!< e.g. "#00ff32"
    const char*		getStdStr(bool withhash=true,
	    			  int transpopt=0) const;
    			//!< transpopt -1=opacity 0=not 1=transparency

    void		fill(char*) const;
    bool		use(const char*);

    static Color	Black()		{ return  Color( 0, 0, 0, 0 ); }
    static Color	White()		{ return  Color( 255, 255, 255, 0 ); }
    static Color	DgbColor()	{ return  Color( 0, 240, 0, 0 ); }	
    static Color	LightGrey()	{ return  Color( 230, 230, 230, 0 ); }
    static Color	NoColor()	{ return  Color( 0, 0, 0, 255 ); }

    static unsigned char getUChar( float v );

    static int		nrStdDrawColors();
    static Color	stdDrawColor(int);

protected:

    unsigned int	col_;
};


#endif
