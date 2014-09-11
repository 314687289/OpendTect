#ifndef odgraphicsitem_h
#define odgraphicsitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include <QGraphicsItem>
#include <QPolygonF>
#include <QString>
#include <QTextOption>
#include <QMutex>
#include <QWaitCondition>
#include <QFont>

#include "draw.h"

class uiPixmap;

static int ODGraphicsType = 100000;

class ODGraphicsPointItem : public QAbstractGraphicsShapeItem
{
public:
    				ODGraphicsPointItem();

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);

    void 			drawPoint(QPainter*);
    void			setHighLight( bool hl )
				{ highlight_ = hl ; }
    void			setColor( const Color& col )
				{ pencolor_ = col ; }

    virtual int			type() const	{ return ODGraphicsType+1; }

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
    static void 		drawMarker(QPainter&,MarkerStyle2D::Type,
					   float,float);

    void			setMarkerStyle(const MarkerStyle2D&);
    void			setFill( bool fill )	  { fill_ = fill; }
    void			setFillColor( const Color& col )
    				{ fillcolor_ = col; }
    void			setSideLength( int side ) { side_ = side; }

    virtual int			type() const	{ return ODGraphicsType+2; }

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
				ODGraphicsPixmapItem(const uiPixmap&);

    void                        paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);

    virtual int			type() const	{ return ODGraphicsType+3; }

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

    virtual int			type() const	{ return ODGraphicsType+4; }

protected:
    ArrowStyle			arrowstyle_;
    int				arrowsz_;
};


class ODGraphicsTextItem : public QAbstractGraphicsShapeItem
{
public:
				ODGraphicsTextItem();

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);

    void			setText(const QString&);
    void			setFont(const QFont&);
    QFont			getFont() const;

    void			setVAlignment(const Qt::Alignment&);
    void			setHAlignment(const Qt::Alignment&);

    virtual int			type() const	{ return ODGraphicsType+5; }

protected:
    void			updateRect();
    QPointF			getAlignment() const;

    QFont			font_;
    QString			text_;
    Qt::Alignment		hal_;
    Qt::Alignment		val_;
};


class ODGraphicsPolyLineItem : public QAbstractGraphicsShapeItem
{
public:
				ODGraphicsPolyLineItem();

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);

    void			setPolyLine(const QPolygonF&,bool closed);
    void			setFillRule(Qt::FillRule);
    bool			isEmpty() const;
    void			setEmpty();

    virtual int			type() const	{ return ODGraphicsType+6; }

protected:

    bool			closed_;
    QPolygonF			qpolygon_;
    Qt::FillRule 		fillrule_;
};


class ODGraphicsDynamicImageItem : public QGraphicsItem, public CallBacker
{
public:
				ODGraphicsDynamicImageItem();
				~ODGraphicsDynamicImageItem();

    QRectF			boundingRect() const { return bbox_; }
    void			paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);

    void			setImage(bool isdynamic,const QImage&,
					 const QRectF&);
    bool			updateResolution(const QPainter*);
    const QRectF&		wantedWorldRect() const;
    const QSize&		wantedScreenSize() const;

    virtual int			type() const	{ return ODGraphicsType+7; }

    Notifier<ODGraphicsDynamicImageItem>	wantsData;

protected:

    QRectF			wantedwr_;
    QSize			wantedscreensz_;

    QMutex			imagelock_;
    QWaitCondition		imagecond_;
    bool			updatedynpixmap_;
    QImage			dynamicimage_;
    QRectF			dynamicimagebbox_;
    bool			dynamicrev_[2];
    bool			updatebasepixmap_;
    QImage			baseimage_;
    QRectF			bbox_;
    bool			baserev_[2];

    PtrMan<QPixmap>		basepixmap_;	//Only access in paint
    PtrMan<QPixmap>		dynamicpixmap_; //Only access in paint
    QRectF			dynamicpixmapbbox_; //Only access in paint

};

#endif
