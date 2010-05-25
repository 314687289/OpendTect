/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id: pixmap.cc,v 1.41 2010-05-25 03:45:52 cvsnanne Exp $";

#include "pixmap.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "bufstringset.h"
#include "color.h"
#include "coltabindex.h"
#include "coltabsequence.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "separstr.h"
#include "uirgbarray.h"

#include <QPixmap>
#include <QBitmap>
#include <QColor>
#include <QImageWriter>


ioPixmap::ioPixmap( const ioPixmap& pm )
    : qpixmap_(new QPixmap(*pm.qpixmap_))
    , srcname_(pm.srcname_)
{
}


ioPixmap::ioPixmap( const uiRGBArray& rgbarr )
    : qpixmap_(new QPixmap)
    , srcname_("[uiRGBArray]")
{
    convertFromRGBArray( rgbarr );
}


ioPixmap::ioPixmap( const char* xpm[] )
    : qpixmap_(new QPixmap(xpm))
    , srcname_("[xpm]")
{
}


ioPixmap::ioPixmap( int w, int h )
    : qpixmap_(new QPixmap( w<2 ? 1 : w, h<2 ? 2 : h ))
    , srcname_("[created]")
{
    qpixmap_->fill( QColor(0,0,0,0) );
}


ioPixmap::ioPixmap( const QPixmap& pm )
    : qpixmap_(new QPixmap(pm))
{
}


ioPixmap::ioPixmap( const char* fnm, const char* fmt )
    : qpixmap_(0)
    , srcname_(fnm)
{
    if ( srcname_.isEmpty() )
	return;

    if ( fmt )
    {
	FileMultiString fms( fnm );
	fms += fmt;
	srcname_ = fms;
    }

    BufferString fname( srcname_ );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
    {
	fp.setPath( GetSettingsFileName("icons") );
	fname = fp.fullPath();
	if ( !File::exists(fname) )
	{
	    fp.setPath( mGetSetupFileName("icons.cur") );
	    fname = fp.fullPath();
	    if ( !File::exists(fname) )
	    {
		fp.setPath( mGetSetupFileName("icons.Default") );
		fname = fp.fullPath();
	    }
	}
    }

    qpixmap_ = new QPixmap( fname.buf(), fmt );
}

    
ioPixmap::ioPixmap( const ColTab::Sequence& ctabin, int width, int height,
       		    bool horizontal )
    : qpixmap_(0)
    , srcname_("[colortable]")
{
    bool validsz = true;
    if ( width < 2 ) { width = 1; validsz = false; }
    if ( height < 2 ) { height = 1; validsz = false; }

    if ( ctabin.size() == 0 || !validsz )
    {
	qpixmap_ = new QPixmap( width, height );
	qpixmap_->fill( QColor(0,0,0,0) );
	return;
    }

    uiRGBArray rgbarr( false );
    rgbarr.setSize( width, height );
    if ( horizontal )
    {
	ColTab::IndexedLookUpTable table( ctabin, width );
	for ( int idx1=0; idx1<rgbarr.getSize(true); idx1++ )
	{
	    const Color color = table.colorForIndex( idx1 );
	    for ( int idx2=0; idx2<rgbarr.getSize(false); idx2++ )
		rgbarr.set( idx1, idx2, color );
	}
    }
    else // vertical colorbar
    {
	ColTab::IndexedLookUpTable table( ctabin, height );
	for ( int idx1=0; idx1<rgbarr.getSize(false); idx1++ )
	{
	    const Color color = table.colorForIndex( idx1 );
	    for ( int idx2=0; idx2<rgbarr.getSize(true); idx2++ )
		rgbarr.set( idx2, idx1, color );
	}
    }

    qpixmap_ = new QPixmap;
    convertFromRGBArray( rgbarr );
}


ioPixmap::~ioPixmap()
{
    releaseDrawTool();
    delete qpixmap_;
}


void ioPixmap::convertFromRGBArray( const uiRGBArray& rgbarr )
{
    releaseDrawTool();

    if ( !qpixmap_ ) qpixmap_ = new QPixmap;
    *qpixmap_ = QPixmap::fromImage( rgbarr.qImage(), Qt::OrderedAlphaDither );
}    


QPaintDevice* ioPixmap::qPaintDevice()
{ return qpixmap_; }

int ioPixmap::width() const
{ return qpixmap_->width(); }

int ioPixmap::height() const
{ return qpixmap_->height(); }

bool ioPixmap::isEmpty() const
{ return !qpixmap_ || qpixmap_->isNull(); }

void ioPixmap::fill( const Color& col )
{ qpixmap_->fill( QColor(col.r(),col.g(),col.b()) ); }


bool ioPixmap::save( const char* fnm, const char* fmt, int quality ) const
{ return qpixmap_ ? qpixmap_->save( fnm, fmt, quality ) : false; }


void ioPixmap::supportedImageFormats( BufferStringSet& list )
{
    QList<QByteArray> qlist = QImageWriter::supportedImageFormats();
    for ( int idx=0; idx<qlist.size(); idx++ )
	list.add( qlist[idx].constData() );
}


// ----- ioBitmap -----
ioBitmap::ioBitmap( const char* filenm, const char * format )
{
    qpixmap_ = new QBitmap( filenm, format );
    srcname_ = filenm;
}


QBitmap* ioBitmap::Bitmap() { return (QBitmap*)qpixmap_; }
const QBitmap* ioBitmap::Bitmap() const { return (const QBitmap*)qpixmap_; }


void supportedImageFormats( BufferStringSet& imageformats )
{
    QList<QByteArray> imgfrmts = QImageWriter::supportedImageFormats();
    for ( int idx=0; idx<imgfrmts.size(); idx++ )
	imageformats.add( imgfrmts[idx].data() );
}
