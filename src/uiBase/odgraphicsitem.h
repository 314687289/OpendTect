#ifndef odgraphicsitem_h
#define odgraphicsitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: odgraphicsitem.h,v 1.16 2012-04-06 12:17:38 cvskris Exp $
________________________________________________________________________

-*/

#include <QGraphicsItem>
#include <QPolygonF>
#include <QString>
#include <QTextOption>
#include <QMutex>

#include "draw.h"

class ioPixmap;

class ODGraphicsPointItem : public QAbstractGraphicsShapeItem
{
public:
    				ODGraphicsPointItem()
				    : QAbstractGraphicsShapeItem()
				    , highlight_(false)
				    , penwidth_(2)
				    , pencolor_(Color::Black())	{}

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			drawPoint(QPainter*);
    void			setHighLight( bool hl )
				{ highlight_ = hl ; }
    void			setColor( const Color& col )
				{ pencolor_ = col ; }

protected:
    bool			highlight_;
    int				penwidth_;
    Color			pencolor_;
};



class ODGraphicsMarkerItem : public QAbstractGraphicsShapeItem
{
public:
    				ODGraphicsMarkerItem();
    virtual			~ODGraphicsMarkerItem();

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    static void 		drawMarker(QPainter&,MarkerStyle2D::Type,float,float);
    void			setMarkerStyle(const MarkerStyle2D&);
    void			setFill( bool fill )	  { fill_ = fill; }
    void			setFillColor( const Color& col )
    				{ fillcolor_ = col; }
    void			setSideLength( int side ) { side_ = side; }

protected:
    QRectF			boundingrect_;
    MarkerStyle2D*		mstyle_;	
    Color			fillcolor_;	
    bool			fill_;	
    int 			side_;	
};


class ODGraphicsPixmapItem : public QGraphicsPixmapItem
{
public:
    				ODGraphicsPixmapItem();
    				ODGraphicsPixmapItem(const ioPixmap&);
    void                        paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);
};


class ODGraphicsArrowItem : public QAbstractGraphicsShapeItem
{
public:
    				ODGraphicsArrowItem();

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			drawArrow(QPainter&);
    double 			getAddedAngle(double,float);
    QPoint 			getEndPoint(const QPoint&,double,double);
    void 			drawArrowHead(QPainter&,const QPoint&,
	    				      const QPoint&);
    void			setArrowStyle( const ArrowStyle& arrowstyle )
    				{ arrowstyle_ = arrowstyle ; }
    void			setArrowSize( const int arrowsz )
    				{ arrowsz_ = arrowsz ; }
    void			setLineStyle(QPainter&,const LineStyle&);

protected:
    ArrowStyle			arrowstyle_;
    int				arrowsz_;
};


class ODGraphicsTextItem : public QGraphicsTextItem
{
public:
    				ODGraphicsTextItem();

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			setTextAlignment(Alignment);
    void			setText(const char*);

protected:
    QRectF			boundingrect_;
    QString			text_;
    QTextOption			alignoption_;
};


class ODGraphicsPolyLineItem : public QAbstractGraphicsShapeItem
{
public:
				ODGraphicsPolyLineItem();

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void			setPolyLine( const QPolygonF& polygon )
    				{
				    prepareGeometryChange();
				    qpolygon_ = polygon;
				}
    bool			isEmpty() const { return qpolygon_.isEmpty(); }
    void			setEmpty() 	{ qpolygon_.clear(); }

protected:
    QPolygonF			qpolygon_;
};


class ODGraphicsDynamicImageItem : public QGraphicsItem, public CallBacker
{
public:
			ODGraphicsDynamicImageItem();

    void		setImage( bool isdynamic, const QImage& image,
	    			  const QRectF& rect );
    const		QRectF& wantedWorldRect() const { return wantedwr_; }
    const QSize&	wantedScreenSize() const { return wantedscreensz_; }

    QRectF		boundingRect() const { return bbox_; }

    void		paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		      QWidget*);

    bool		updateResolution(const QPainter*);

    Notifier<ODGraphicsDynamicImageItem>	wantsData;
protected:

    QRectF			wantedwr_;
    QSize			wantedscreensz_;

    bool			updatedynpixmap_;
    QMutex			dynamiclock_;
    QImage			dynamicimage_;
    QRectF			dynamicimagebbox_;

    PtrMan<QPixmap>		dynamicpixmap_; //Only access in paint
    QRectF			dynamicpixmapbbox_; //Only access in paint

    QPixmap			basepixmap_;
    QRectF			bbox_;
};


#endif
