#ifndef uicolor_h
#define uicolor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          22/05/2000
 RCS:           $Id: uicolor.h,v 1.15 2008-05-05 05:42:18 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "uigroup.h"

class uiLabel;
class uiPushButton;
class uiCheckBox;


/*! \brief pops a selector box to select a new color 
     \return true if new color selected
*/
bool  	selectColor(Color&,uiParent* parnt=0,const char* seltxt=0,
		    bool withtransp=false); 

// To be used by cmddriver to select a color while closing the QColorDialog
void		setExternalColor( const Color& );
static Color*	externalcolor = 0;


/*! \brief small element for color selection. Has no text label.

  The label in this class is for displaying the current color. Another label
  may be created if you specify the lbltxt.
 
 */

class uiColorInput : public uiGroup
{
public:
    class Setup
    {
    public:
			Setup(const Color& col)
			    : color_(col)
			    , lbltxt_("")
			    , withcheck_(false)
			    , dlgtitle_("Select color")
			    , withalpha_(false)
			{}

	mDefSetupMemb(Color,color)
	mDefSetupMemb(BufferString,lbltxt)
	mDefSetupMemb(bool,withcheck)
	mDefSetupMemb(BufferString,dlgtitle)
	mDefSetupMemb(bool,withalpha)

    };

    				uiColorInput(uiParent*,const Setup&,
					     const char* nm=0);

    const Color&		color() const	{ return color_; }
    void			setColor(const Color&);
    bool			doDraw() const;
    void			setDoDraw(bool);

    void			enableAlphaSetting( bool yn )
				{ withalpha_ = yn; }

    Notifier<uiColorInput> 	colorchanged;
    Notifier<uiColorInput> 	dodrawchanged;

protected:

    uiPushButton*		colbut_;
    uiCheckBox*			dodrawbox_;

    Color			color_;
    BufferString		dlgtxt_;
    bool			withalpha_;

    void			selCol(CallBacker*);
    void			dodrawSel(CallBacker*);
};

#endif
