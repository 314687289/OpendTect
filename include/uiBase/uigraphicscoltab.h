#ifndef uigraphicscoltab_h
#define uigraphicscoltab_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigraphicsitem.h"
#include "coltabsequence.h"

#include "draw.h"

class uiPixmapItem;
class uiRectItem;
class uiTextItem;
namespace ColTab { class MapperSetup; }


mExpClass(uiBase) uiColTabItem : public uiGraphicsItemGroup
{
public:

    mExpClass(uiBase) Setup
    {
    public:
			Setup( bool h ) //!< horizontal?
			    : hor_(h)
			    , sz_(h?100:25,h?25:100)
			    , startal_(Alignment::HCenter,
				       h?Alignment::Top:Alignment::VCenter)
			    , stopal_(Alignment::HCenter,
				      h?Alignment::Bottom:Alignment::VCenter)
			    , startalong_(false)
			    , stopalong_(false)		{}
	mDefSetupMemb(bool,hor)
	mDefSetupMemb(uiSize,sz)
	mDefSetupMemb(Alignment,startal)
	mDefSetupMemb(Alignment,stopal)
	mDefSetupMemb(bool,startalong)	// put number along color bar
	mDefSetupMemb(bool,stopalong)	// put number along color bar
    };

    			uiColTabItem(const Setup&);
			~uiColTabItem();
    Setup&		setup()		{ return setup_; }
    const Setup&	setup() const	{ return setup_; }

    void		setColTab(const char* nm);
    void		setColTabSequence(const ColTab::Sequence&);
    void		setColTabMapperSetup(const ColTab::MapperSetup&);

    uiPoint		getPixmapPos() const	{ return curpos_; }
    void		setPixmapPos(const uiPoint&);

    void		setupChanged();

protected:

    Setup		setup_;
    ColTab::Sequence	ctseq_;
    uiPoint		curpos_;

    uiPixmapItem*	ctseqitm_;
    uiRectItem*		borderitm_;
    uiTextItem*		minvalitm_;
    uiTextItem*		maxvalitm_;

    virtual void	stPos(float,float);
    void		setPixmap();
    void		setPixmapPos();

};

#endif

