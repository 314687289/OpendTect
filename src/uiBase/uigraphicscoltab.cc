/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicscoltab.cc,v 1.5 2009-07-22 16:01:38 cvsbert Exp $";


#include "uigraphicscoltab.h"
#include "uigraphicsitemimpl.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "draw.h"
#include "pixmap.h"

uiColTabItem::uiColTabItem( const uiColTabItem::Setup& su )
    : uiGraphicsItemGroup()
    , setup_(su)
{
    ctseqitm_ = new uiPixmapItem();
    minvalitm_ = new uiTextItem( "0", mAlignment(HCenter,Top) );
    maxvalitm_ = new uiTextItem( "1", mAlignment(HCenter,Bottom) );
    add( ctseqitm_ ); add( minvalitm_ ); add( maxvalitm_ );

    setColTabSequence( ColTab::Sequence("") );
}


uiColTabItem::~uiColTabItem()
{
    removeAll( true );
}


void uiColTabItem::setColTabSequence( const ColTab::Sequence& ctseq )
{
    ctseq_ = ctseq;
    setPixmap();
}


void uiColTabItem::setPixmap()
{
    ioPixmap pm( ctseq_, setup_.sz_.hNrPics(), setup_.sz_.vNrPics() );
    ctseqitm_->setPixmap( pm );
}


void uiColTabItem::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    Interval<float> rg( ms.start_, ms.start_+ms.width_ );
    minvalitm_->setText( toString(rg.start) );
    maxvalitm_->setText( toString(rg.stop) );
}


void uiColTabItem::setPixmapPos( const uiPoint& pt )
{
    curpos_ = pt;
    setPixmapPos();
}


void uiColTabItem::setPixmapPos()
{
    ctseqitm_->setPos( curpos_ );

    const uiRect rect( curpos_, setup_.sz_ );
    const int dist = 2;
    const uiRect drect( uiPoint(curpos_.x-dist,curpos_.y-dist),
	    	 uiSize(setup_.sz_.width()+2*dist,setup_.sz_.height()+2*dist) );
    const uiPoint center( rect.centre() );
#   define mSetAl(itm,h,v) itm->setAlignment( Alignment(h,v) )

    if ( setup_.hor_ )
    {
	const int starty =
	    setup_.startal_.vPos() == Alignment::VCenter ? center.y
	 : (setup_.startal_.vPos() == Alignment::Top ?     drect.top()
						     :     drect.bottom());
	if ( setup_.startalong_ )
	{
	    mSetAl( minvalitm_, Alignment::Left,
		    Alignment::opposite(setup_.startal_.vPos()) );
	    minvalitm_->setPos( rect.left(), starty );
	}
	else
	{
	    mSetAl( minvalitm_, Alignment::Right, setup_.startal_.vPos() );
	    minvalitm_->setPos( drect.left(), starty );
	}
	const int stopy =
	    setup_.stopal_.vPos() == Alignment::VCenter ? center.y
	 : (setup_.stopal_.vPos() == Alignment::Top ?     drect.top()
						    :     drect.bottom());
	if ( setup_.stopalong_ )
	{
	    mSetAl( maxvalitm_, Alignment::Right,
		    Alignment::opposite(setup_.stopal_.vPos()) );
	    maxvalitm_->setPos( rect.right(), stopy );
	}
	else
	{
	    mSetAl( maxvalitm_, Alignment::Left, setup_.stopal_.vPos() );
	    maxvalitm_->setPos( drect.right(), stopy );
	}
    }
    else
    {
	const int startx =
	    setup_.startal_.hPos() == Alignment::HCenter ? center.x
	 : (setup_.startal_.hPos() == Alignment::Left ?    drect.left()
						      :    drect.right());
	const Alignment::HPos oppal = Alignment::opposite(
						setup_.startal_.hPos() );

	if ( setup_.startalong_ )
	{
	    mSetAl( minvalitm_, oppal, Alignment::Bottom );
	    minvalitm_->setPos( startx, rect.bottom() );
	}
	else
	{
	    mSetAl( minvalitm_, oppal, Alignment::Top );
	    minvalitm_->setPos( startx, drect.bottom() );
	}
	const int stopx =
	    setup_.stopal_.hPos() == Alignment::HCenter ? center.x
	 : (setup_.stopal_.hPos() == Alignment::Left ?    drect.left()
						     :    drect.right());
	if ( setup_.stopalong_ )
	{
	    mSetAl( maxvalitm_, oppal, Alignment::Top );
	    minvalitm_->setPos( startx, rect.bottom() );
	}
	else
	{
	    mSetAl( maxvalitm_, oppal, Alignment::Bottom );
	    minvalitm_->setPos( startx, drect.bottom() );
	}
    }
}


void uiColTabItem::setupChanged()
{
    setPixmap();
    setPixmapPos();
}


void uiColTabItem::setPos( const uiPoint& pt )
{
    setPixmapPos( pt );
    curpos_ += curpos_;
    curpos_ -= boundingRect().topLeft();
    setPixmapPos();
}
