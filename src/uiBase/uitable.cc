/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          12/02/2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uitable.cc,v 1.94 2010-09-24 12:03:52 cvsnanne Exp $";


#include "uitable.h"

#include "uifont.h"
#include "uimenu.h"
#include "pixmap.h"
#include "uilabel.h"
#include "uiobjbody.h"
#include "uicombobox.h"
#include "convert.h"
#include "bufstringset.h"
#include "i_layoutitem.h"
#include "i_qtable.h"

#include <QHeaderView>
#include <QMouseEvent>


class CellObject
{
    public:
			CellObject( QWidget* qw, uiObject* obj,
				    const RowCol& rc )
			    : qwidget_(qw)
			    , object_(obj)
			    , rowcol_(rc)    {}
			~CellObject();

    uiObject*		object_;
    QWidget*		qwidget_;
    RowCol		rowcol_;
};


CellObject::~CellObject()
{
    mDynamicCastGet(uiGroupObj*,grpobj,object_);

    if ( grpobj && grpobj->group() )
	delete grpobj->group();
    else
	delete object_;
}


class uiTableBody : public uiObjBodyImpl<uiTable,QTableWidget>
{
public:
			uiTableBody(uiTable&,uiParent*,const char*,int,int);
			~uiTableBody();

    void		setLines(int);
    virtual int 	nrTxtLines() const;

    QTableWidgetItem*	getItem(const RowCol&,bool createnew=true);

    void		clearCellObject(const RowCol&);
    uiObject*		getCellObject(const RowCol&) const;
    void		setCellObject(const RowCol&,uiObject*);
    RowCol		getCell(uiObject*);

    int			maxSelectable() const;
    uiTable::SelectionBehavior getSelBehavior() const;
    
protected:
    virtual void	mouseReleaseEvent(QMouseEvent*);

    ObjectSet<CellObject> cellobjects_;

    BoolTypeSet		columnsreadonly;
    BoolTypeSet		rowsreadonly;

private:

    i_tableMessenger&	messenger_;

};


uiTableBody::uiTableBody( uiTable& handle, uiParent* parnt, const char* nm,
			  int nrows, int ncols )
    : uiObjBodyImpl<uiTable,QTableWidget>(handle,parnt,nm)
    , messenger_ (*new i_tableMessenger(this,&handle))
{
    if ( nrows >= 0 ) setLines( nrows );
    if ( ncols >= 0 ) setColumnCount( ncols );

    QHeaderView* vhdr = verticalHeader();
// TODO: Causes tremendous performance delay in Qt 4.4.1;
//       For now use uiTable::resizeRowsToContents() in stead.
//    vhdr->setResizeMode( QHeaderView::ResizeToContents );

    QHeaderView* hhdr = horizontalHeader();
    hhdr->setResizeMode( QHeaderView::Stretch );

    setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel );

    setMouseTracking( true );
}


uiTableBody::~uiTableBody()
{
    deepErase( cellobjects_ );
    delete &messenger_;
}


void uiTableBody::mouseReleaseEvent( QMouseEvent* event )
{
    if ( !event ) return;

    if ( event->button() == Qt::RightButton )
	handle_.buttonstate_ = OD::RightButton;
    else if ( event->button() == Qt::LeftButton )
	handle_.buttonstate_ = OD::LeftButton;
    else
	handle_.buttonstate_ = OD::NoButton;

    QAbstractItemView::mouseReleaseEvent( event );
    handle_.buttonstate_ = OD::NoButton;
}


void uiTableBody::setLines( int prefnrlines )
{ 
    setRowCount( prefnrlines );
    if ( !finalised() && prefnrlines > 0 )
    {
	QHeaderView* vhdr = verticalHeader();
	const QSize qsz = vhdr->sizeHint();
	const int rowh = rowHeight(0) + 1;
	const int prefh = rowh*prefnrlines + qsz.height();
	setPrefHeight( mMIN(prefh,200) );
    }
}


int uiTableBody::nrTxtLines() const
{ return rowCount()>=0 ? rowCount()+1 : 7; }


QTableWidgetItem* uiTableBody::getItem( const RowCol& rc, bool createnew )
{
    QTableWidgetItem* itm = item( rc.row, rc.col );
    if ( !itm && createnew )
    {
	itm = new QTableWidgetItem;
	setItem( rc.row, rc.col, itm );
    }

    return itm;
}


void uiTableBody::setCellObject( const RowCol& rc, uiObject* obj )
{
    QWidget* qwidget = obj->body()->qwidget();
    setCellWidget( rc.row, rc.col, qwidget );
    cellobjects_ += new CellObject( qwidget, obj, rc );
}


uiObject* uiTableBody::getCellObject( const RowCol& rc ) const
{
    QWidget* qw = cellWidget( rc.row, rc.col );
    if ( !qw ) return 0;

    uiObject* obj = 0;
    for ( int idx=0; idx<cellobjects_.size(); idx++ )
    {
	if ( cellobjects_[idx]->qwidget_ == qw )
	{
	    obj = cellobjects_[idx]->object_;
	    break;
	}
    }

    return obj;
}


RowCol uiTableBody::getCell( uiObject* obj )
{
    for ( int idx=0; idx<cellobjects_.size(); idx++ )
    {
	if ( cellobjects_[idx]->object_ == obj )
	    return cellobjects_[idx]->rowcol_;
    }

    return RowCol( -1, -1 );
}


void uiTableBody::clearCellObject( const RowCol& rc )
{
    QWidget* qw = cellWidget( rc.row, rc.col );
    if ( !qw ) return;

    CellObject* co = 0;
    for ( int idx=0; idx<cellobjects_.size(); idx++ )
    {
	if ( cellobjects_[idx]->qwidget_ == qw )
	{
	    co = cellobjects_[idx];
	    break;
	}
    }

    setCellWidget( rc.row, rc.col, 0 );
    if ( co )
    {
	cellobjects_ -= co;
	delete co;
    }
}


uiTable::SelectionBehavior uiTableBody::getSelBehavior() const
{
    return (uiTable::SelectionBehavior) selectionBehavior();
}


int uiTableBody::maxSelectable() const
{
    if ( selectionMode()==QAbstractItemView::NoSelection )
	return 0;
    if ( selectionMode()==QAbstractItemView::SingleSelection )
       	return 1;
    if ( getSelBehavior()==uiTable::SelectRows )
	return rowCount();
    if ( getSelBehavior()==uiTable::SelectColumns )
	return columnCount();

    return rowCount()*columnCount();
}


uiTable::uiTable( uiParent* p, const Setup& s, const char* nm )
    : uiObject(p,nm,mkbody(p,nm,s.size_.row,s.size_.col))
    , setup_(s)
    , buttonstate_(OD::NoButton)
    , valueChanged(this)
    , rightClicked(this)
    , leftClicked(this)
    , doubleClicked(this)
    , rowInserted(this)
    , colInserted(this)
    , rowDeleted(this)
    , colDeleted(this)
    , selectionChanged(this)
    , rowClicked(this)
    , columnClicked(this)
{
    setFont( FontList().get(FontData::key(FontData::Fixed)) );
    rightClicked.notify( mCB(this,uiTable,popupMenu) );
    setGeometry.notify( mCB(this,uiTable,geometrySet_) );

    setStretch( 2, 2 );

    setSelectionMode( s.selmode_ );
    if ( s.defrowlbl_ )
	setDefaultRowLabels();
    if ( s.defcollbl_ )
	setDefaultColLabels();

    QHeaderView* hhdr = body_->horizontalHeader();
    hhdr->setMinimumSectionSize( (int)(s.mincolwdt_*body_->fontWdt()) );
}


uiTableBody& uiTable::mkbody( uiParent* p, const char* nm, int nr, int nc )
{
    body_ = new uiTableBody( *this, p, nm, nr, nc );
    return *body_;
}


uiTable::~uiTable()
{
    deepErase( selranges_ );
}


void uiTable::showGrid( bool yn )
{ body_->setShowGrid( yn ); }

bool uiTable::gridShown() const
{ return body_->showGrid(); }


void uiTable::setDefaultRowLabels()
{
    const int nrrows = nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	BufferString lbl( setup_.rowdesc_ ); lbl += " ";
	lbl += ( idx + setup_.defrowstartidx_ );
	setRowLabel( idx, lbl );
    }
}


void uiTable::setDefaultColLabels()
{
    const int nrcols = nrCols();
    for ( int idx=0; idx<nrcols; idx++ )
    {
	BufferString lbl( setup_.coldesc_ ); lbl += " ";
	lbl += idx+1;
	setColumnLabel( idx, lbl );
    }
}

#define updateRow(r) update( true, r )
#define updateCol(c) update( false, c )

void uiTable::update( bool row, int rc )
{
    if ( row && setup_.defrowlbl_ ) setDefaultRowLabels(); 
    else if ( !row && setup_.defcollbl_ ) setDefaultColLabels();

    int max = row ? nrRows() : nrCols();

    if ( rc > max ) rc = max;
    if ( rc < 0 ) rc = 0;

    int r = row ? rc : 0;
    int c = row ? 0 : rc;

    setCurrentCell( RowCol(r,c) );
}


int uiTable::columnWidth( int col ) const
{
    return col == -1 ? 0 : body_->columnWidth(col);
}


int uiTable::rowHeight( int row ) const	
{
    return row == -1 ? 0 : body_->rowHeight( row );
}


void uiTable::setLeftMargin( int width )
{
    QHeaderView* header = body_->verticalHeader();
    if ( !header ) return;

    header->setVisible( width > 0 );
}


void uiTable::setColumnWidth( int col, int w )	
{
    if ( col >= 0 )
	body_->setColumnWidth( col, w );
    else if ( col == -1 )
    {
	for ( int idx=0; idx<nrCols(); idx++ )
	    body_->setColumnWidth( idx, w );
    }
}


void uiTable::setColumnWidthInChar( int col, float w )
{
    const float wdt = w * body_->fontWdt();
    setColumnWidth( col, mNINT(wdt) );
}


void uiTable::setTopMargin( int h )
{
    QHeaderView* header = body_->horizontalHeader();
    if ( !header ) return;

    header->setVisible( h > 0 );
}


void uiTable::setRowHeight( int row, int h )
{
    if ( row >= 0 )
	body_->setRowHeight( row, h );
    else if ( row == -1 )
    {
	for ( int idx=0; idx<nrRows(); idx++ )
	    body_->setRowHeight( idx, h );
    }
}


void uiTable::setRowHeightInChar( int row, float h )
{
    float hgt = h * body_->fontHgt();
    setRowHeight( row, mNINT(hgt) );
}


void uiTable::insertRows( int row, int cnt )
{
    for ( int idx=0; idx<cnt; idx++ )
	body_->insertRow( row );

    updateRow( row );
}


void uiTable::insertColumns( int col, int cnt )
{
    for ( int idx=0; idx<cnt; idx++ )
	body_->insertColumn( col );

    updateCol( col );
}


void uiTable::removeRCs( const TypeSet<int>& idxs, bool col )
{
    if ( idxs.size() < 1 ) return;
    for ( int idx=idxs.size()-1; idx>=0; idx-- )
	col ? removeColumn( idxs[idx] ) : removeRow( idxs[idx] );
}


void uiTable::removeRow( int row )
{ 
    for ( int col=0; col<nrCols(); col++ )
	clearCellObject( RowCol(row,col) );
    body_->removeRow( row );
    updateRow(row);
}


void uiTable::removeColumn( int col )
{ body_->removeColumn( col );  updateCol(col); }

void uiTable::removeRows( const TypeSet<int>& idxs )
{ removeRCs( idxs, false ); }

void uiTable::removeColumns( const TypeSet<int>& idxs )
{ removeRCs( idxs, true ); }

void uiTable::setNrRows( int nr )
{ body_->setLines( nr ); updateRow(0); }

void uiTable::setNrCols( int nr )
{ body_->setColumnCount( nr );  updateCol(0); }

int uiTable::nrRows() const		{ return body_->rowCount(); }
int uiTable::nrCols() const		{ return body_->columnCount(); }

void uiTable::clearCell( const RowCol& rc )
{
    QTableWidgetItem* itm = body_->takeItem( rc.row, rc.col );
    delete itm;
}


void uiTable::setCurrentCell( const RowCol& rc, bool noselection )
{
    if ( noselection )
	body_->setCurrentCell( rc.row, rc.col, QItemSelectionModel::NoUpdate );
    else
	body_->setCurrentCell( rc.row, rc.col );
}


const char* uiTable::text( const RowCol& rc ) const
{
    const uiObject* cellobj = getCellObject( rc );
    if ( cellobj )
    {
	mDynamicCastGet(const uiComboBox*,cb,cellobj)
	if ( !cb )
	{
	    pErrMsg("TODO: unknown table cell obj: add it!");
	    return cellobj->name();
	}
	return cb->text();
    }

    static BufferString rettxt;
    QTableWidgetItem* itm = body_->item( rc.row, rc.col );
    rettxt = itm ? itm->text().toAscii().data() : "";
    return rettxt;
}


void uiTable::setText( const RowCol& rc, const char* txt )
{
    uiObject* cellobj = getCellObject( rc );
    if ( !cellobj )
    {
	QTableWidgetItem* itm = body_->getItem( rc );
	itm->setText( txt );
    }
    else
    {
	mDynamicCastGet(uiComboBox*,cb,cellobj)
	if ( !cb )
	    pErrMsg("TODO: unknown table cell obj: add it!");
	else
	    cb->setText( txt );
    }
}


static QAbstractItemView::EditTriggers triggers_ro =
					QAbstractItemView::NoEditTriggers;
static QAbstractItemView::EditTriggers triggers =
					QAbstractItemView::EditKeyPressed |
					QAbstractItemView::AnyKeyPressed |
					QAbstractItemView::DoubleClicked;

void uiTable::setTableReadOnly( bool yn )
{
    body_->setEditTriggers( yn ? triggers_ro : triggers );
}


bool uiTable::isTableReadOnly() const
{
    return false;
}


static Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEditable |
				Qt::ItemIsEnabled;
static Qt::ItemFlags flags_ro = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

void uiTable::setColumnReadOnly( int col, bool yn )
{
    for ( int row=0; row<nrRows(); row++ )
    {
	QTableWidgetItem* itm = body_->getItem( RowCol(row,col), true );
	if ( itm ) itm->setFlags( yn ? flags_ro : flags );
    }
}


void uiTable::setCellReadOnly( const RowCol& rc, bool yn )
{
    QTableWidgetItem* itm = body_->item( rc.row, rc.col );
    if ( itm ) itm->setFlags( yn ? flags_ro : flags );
}


bool uiTable::isCellReadOnly( const RowCol& rc ) const
{
    QTableWidgetItem* itm = body_->item( rc.row, rc.col );
    return itm && !itm->flags().testFlag( Qt::ItemIsEditable );
}


void uiTable::setRowReadOnly( int row, bool yn )
{
    for ( int col=0; col<nrCols(); col++ )
    {
	QTableWidgetItem* itm = body_->getItem( RowCol(row,col), true );
	if ( itm ) itm->setFlags( yn ? flags_ro : flags );
    }
}


bool uiTable::isColumnReadOnly( int col ) const
{
    int nritems = 0;
    int nrro = 0;
    for ( int row=0; row<nrRows(); row++ )
    {
	QTableWidgetItem* itm = body_->item( row, col );
	if ( itm )
	{
	    nritems ++;
	    if ( !itm->flags().testFlag(Qt::ItemIsEditable) )
		nrro++;
	}
    }

    return nritems && nritems==nrro;
}


bool uiTable::isRowReadOnly( int row ) const
{
    int nritems = 0;
    int nrro = 0;
    for ( int col=0; col<nrCols(); col++ )
    {
	QTableWidgetItem* itm = body_->item( row, col );
	if ( itm )
	{
	    nritems++;
	    if ( !itm->flags().testFlag(Qt::ItemIsEditable) )
		nrro++;
	}
    }

    return nritems && nritems==nrro;
}


void uiTable::hideColumn( int col, bool yn )
{
    if ( yn ) body_->hideColumn( col );
    else body_->showColumn( col );
}

void uiTable::hideRow( int col, bool yn )
{
    if ( yn ) body_->hideRow( col );
    else body_->showRow( col );
}

bool uiTable::isColumnHidden( int col ) const
{ return body_->isColumnHidden(col); }

bool uiTable::isRowHidden( int row ) const
{ return body_->isRowHidden(row); }


bool uiTable::isTopHeaderHidden() const
{ return !body_->horizontalHeader()->isVisible(); }

bool uiTable::isLeftHeaderHidden() const
{ return !body_->verticalHeader()->isVisible(); }


void uiTable::resizeHeaderToContents( bool hor )
{
    QHeaderView* hdr = hor ? body_->horizontalHeader() :body_->verticalHeader();
    if ( hdr ) hdr->resizeSections( QHeaderView::ResizeToContents );
}


void uiTable::resizeColumnToContents( int col )
{ body_->resizeColumnToContents( col ); }

void uiTable::resizeColumnsToContents()
{ body_->resizeColumnsToContents(); }

void uiTable::resizeRowToContents( int row )
{ body_->resizeRowToContents( row ); }

void uiTable::resizeRowsToContents()
{ body_->resizeRowsToContents(); }


void uiTable::setColumnResizeMode( ResizeMode mode )
{
    QHeaderView* header = body_->horizontalHeader();
    header->setResizeMode( (QHeaderView::ResizeMode)(int)mode );
}


void uiTable::setRowResizeMode( ResizeMode mode )
{
    QHeaderView* header = body_->verticalHeader();
    header->setResizeMode( (QHeaderView::ResizeMode)(int)mode );
}


void uiTable::setColumnStretchable( int col, bool yn )
{
    QHeaderView* header = body_->horizontalHeader();
    QHeaderView::ResizeMode mode = yn ? QHeaderView::Stretch
				      : QHeaderView::Interactive ;
    header->setResizeMode( header->logicalIndex(col), mode );
}


void uiTable::setRowStretchable( int row, bool yn )
{
    QHeaderView* header = body_->verticalHeader();
    QHeaderView::ResizeMode mode = yn ? QHeaderView::Stretch
				      : QHeaderView::Interactive ;
    header->setResizeMode( header->logicalIndex(row), mode );
}


bool uiTable::isColumnStretchable( int col ) const
{  pErrMsg( "Not impl yet" ); return false; }

bool uiTable::isRowStretchable( int row ) const
{  pErrMsg( "Not impl yet" ); return false; }


void uiTable::setPixmap( const RowCol& rc, const ioPixmap& pm )
{
    QTableWidgetItem* itm = body_->getItem( rc );
    if ( itm ) itm->setIcon( *pm.qpixmap() );
}


void uiTable::setColor( const RowCol& rc, const Color& col )
{
    QColor qcol( col.r(), col.g(), col.b() );
    QTableWidgetItem* itm = body_->getItem( rc );
    if ( itm ) itm->setBackground( qcol );
    body_->setFocus();
}


Color uiTable::getColor( const RowCol& rc ) const
{
    QTableWidgetItem* itm = body_->getItem( rc, false );
    if ( !itm ) return Color(255,255,255);

    const QColor qcol = itm->background().color();
    return Color( qcol.red(), qcol.green(), qcol.blue() );
}


const char* uiTable::rowLabel( int row ) const
{
    static BufferString ret;
    QTableWidgetItem* itm = body_->verticalHeaderItem( row );
    if ( !itm )
	return 0;

    ret = itm->text().toAscii().data();
    return ret;
}


void uiTable::setRowLabel( int row, const char* label )
{
    QTableWidgetItem* itm = body_->verticalHeaderItem( row );
    if ( !itm )
    {
	itm = new QTableWidgetItem;
	body_->setVerticalHeaderItem( row, itm );
    }
    itm->setText( label );
    itm->setToolTip( label );
}


void uiTable::setRowLabels( const char** labels )
{
    BufferStringSet lbls( labels );
    setRowLabels( lbls );
}


void uiTable::setRowLabels( const BufferStringSet& labels )
{
    body_->setLines( labels.size() );
    for ( int i=0; i<labels.size(); i++ )
        setRowLabel( i, *labels[i] );
}


const char* uiTable::columnLabel( int col ) const
{
    static BufferString ret;
    QTableWidgetItem* itm = body_->horizontalHeaderItem( col );
    if ( !itm )
	return 0;

    ret = itm->text().toAscii().data();
    return ret;
}


void uiTable::setColumnLabel( int col, const char* label )
{
    QTableWidgetItem* itm = body_->horizontalHeaderItem( col );
    if ( !itm )
    {
	itm = new QTableWidgetItem;
	body_->setHorizontalHeaderItem( col, itm );
    }
    itm->setText( label );
    itm->setToolTip( label );
}


void uiTable::setColumnLabels( const char** labels )
{
    BufferStringSet lbls( labels );
    setColumnLabels( lbls );
}


void uiTable::setColumnLabels( const BufferStringSet& labels )
{
    body_->setColumnCount( labels.size() );

    for ( int i=0; i<labels.size(); i++ )
        setColumnLabel( i, labels[i]->buf() );
}


void uiTable::setLabelAlignment( Alignment::HPos hal, bool col )
{
    QHeaderView* hdr = col ? body_->horizontalHeader():body_->verticalHeader();
    if ( hdr )
    {
	Alignment al( hal, Alignment::VCenter );
	hdr->setDefaultAlignment( (Qt::Alignment)al.uiValue() );
    }
}


int uiTable::getIntValue( const RowCol& rc ) const
{
    const char* str = text( rc );
    if ( !str || !*str ) return mUdf(int);

    return Conv::to<int>( str );
}


double uiTable::getdValue( const RowCol& rc ) const
{
    const char* str = text( rc );
    if ( !str || !*str ) return mUdf(double);

    return Conv::to<double>(str);
}


float uiTable::getfValue( const RowCol& rc ) const
{
    const char* str = text( rc );
    if ( !str || !*str ) return mUdf(float);

    return Conv::to<float>(str);
}


void uiTable::setValue( const RowCol& rc, int i )
{
    BufferString txt( Conv::to<const char*>(i) );
    setText( rc, txt.buf() );
}


void uiTable::setValue( const RowCol& rc, float f )
{
    if ( mIsUdf(f) )
	setText( rc, "" );
    else
    {
	BufferString txt( Conv::to<const char*>(f) );
	setText( rc, txt.buf() );
    }
}


void uiTable::setValue( const RowCol& rc, double d )
{
    if ( mIsUdf(d) )
	setText( rc, "" );
    else
    {
	BufferString txt( Conv::to<const char*>(d) );
	setText( rc, txt.buf() );
    }
}


void uiTable::setSelectionMode( SelectionMode m )
{
    switch ( m ) 
    {
	case Single:
	    body_->setSelectionMode( QAbstractItemView::SingleSelection );
	    break;
	case Multi:
	    body_->setSelectionMode( QAbstractItemView::ExtendedSelection );
	    break;
	case SingleRow:
	    setSelectionBehavior( uiTable::SelectRows );
	    break;
	default:
	    body_->setSelectionMode( QAbstractItemView::NoSelection );
	    break;
    }
}


void uiTable::setSelectionBehavior( SelectionBehavior sb )
{
    const int sbi = (int)sb;
    body_->setSelectionBehavior( (QAbstractItemView::SelectionBehavior)sbi );
}


void uiTable::editCell( const RowCol& rc, bool replace )
{
    QTableWidgetItem* itm = body_->item( rc.row, rc.col );
    body_->editItem( itm );
}


void uiTable::popupMenu( CallBacker* )
{
    if ( (!setup_.rowgrow_ && !setup_.colgrow_) || setup_.rightclickdisabled_ )
	return;

    uiPopupMenu* mnu = new uiPopupMenu( parent(), "Action" );
    BufferString itmtxt;

    int inscolbef = 0;
    int delcol = 0;
    int inscolaft = 0;
    if ( setup_.colgrow_ )
    {
	if ( setup_.insertcolallowed_ )
	{
	    itmtxt =  BufferString( "Insert ", setup_.coldesc_, " before" );
	    inscolbef = mnu->insertItem( new uiMenuItem(itmtxt), 0 );
	    itmtxt =  BufferString( "Insert ", setup_.coldesc_, " after" );
	    inscolaft = mnu->insertItem( new uiMenuItem(itmtxt), 2 );
	}

	if ( setup_.removecolallowed_ )
	{
	    itmtxt = "Remove "; itmtxt += setup_.coldesc_;
	    delcol = mnu->insertItem( new uiMenuItem(itmtxt), 1 );
	}
    }

    int insrowbef = 0;
    int delrow = 0;
    int insrowaft = 0;
    if ( setup_.rowgrow_ )
    {
	if ( setup_.insertrowallowed_ )
	{
	    itmtxt = BufferString( "Insert ", setup_.rowdesc_, " before" );
	    insrowbef = mnu->insertItem( new uiMenuItem(itmtxt), 3 );
	    itmtxt = BufferString( "Insert ", setup_.rowdesc_, " after" );
	    insrowaft = mnu->insertItem( new uiMenuItem(itmtxt), 5 );
	}

	if ( setup_.removerowallowed_ )
	{
	    itmtxt = "Remove "; itmtxt += setup_.rowdesc_;
	    delrow = mnu->insertItem( new uiMenuItem(itmtxt), 4 );
	}
    }

    int ret = mnu->exec();
    if ( ret<0 ) return;

    const RowCol cur = notifiedCell();
    if ( ret == inscolbef || ret == inscolaft )
    {
	const int offset = (ret == inscolbef) ? 0 : 1;
	newcell_ = RowCol( cur.row, cur.col+offset );
	insertColumns( newcell_, 1 );

	if ( !setup_.defcollbl_ )
	    setColumnLabel( newcell_, toString(newcell_.col) );

	colInserted.trigger();
    }
    else if ( ret == delcol )
    {
	removeColumn( cur.col );
	colDeleted.trigger();
    }
    else if ( ret == insrowbef || ret == insrowaft  )
    {
	const int offset = (ret == insrowbef) ? 0 : 1;
	newcell_ = RowCol( cur.row+offset, cur.col );
	insertRows( newcell_, 1 );

	if ( !setup_.defrowlbl_ )
	    setRowLabel( newcell_, toString(newcell_.row) );

	rowInserted.trigger();
    }
    else if ( ret == delrow )
    {
	removeRow( cur.row );
	rowDeleted.trigger();
    }

    setCurrentCell( newcell_ );
//    updateCellSizes();
}


void uiTable::geometrySet_( CallBacker* cb )
{
//    if ( !mainwin() ||  mainwin()->poppedUp() ) return;
    mCBCapsuleUnpack(uiRect&,sz,cb);

    const uiSize size = sz.getPixelSize();
//    updateCellSizes( &size );
}


void uiTable::updateCellSizes( const uiSize* size )
{
    if ( size ) lastsz = *size;
    else	size = &lastsz;

    if ( !setup_.manualresize_ && body_->layoutItem()->inited() )
    {
	if ( !setup_.fillcol_ )
	    for ( int idx=0; idx < nrCols(); idx++ )
		setColumnStretchable( idx, true );

        if ( !setup_.fillrow_ )
	    for ( int idx=0; idx < nrRows(); idx++ )
		setRowStretchable( idx, true );
    }

    int nc = nrCols();
    if ( nc && setup_.fillrow_ )
    {
	int width = size->hNrPics();
	int availwdt = width - body_->verticalHeader()->frameSize().width()
			 - 2*body_->frameWidth();

	int colwdt = availwdt / nc;

	const int minwdt = (int)(setup_.mincolwdt_ * (float)font()->avgWidth());
	const int maxwdt = (int)(setup_.maxcolwdt_ * (float)font()->avgWidth());

	if ( colwdt < minwdt ) colwdt = minwdt;
	if ( colwdt > maxwdt ) colwdt = maxwdt;

	for ( int idx=0; idx < nc; idx ++ )
	{
	    if ( idx < nc-1 )
		setColumnWidth( idx, colwdt );
	    else 
	    {
		int wdt = availwdt;
		if ( wdt < minwdt )	wdt = minwdt;
		if ( wdt > maxwdt )	wdt = maxwdt;

		setColumnWidth( idx, wdt );
	    }
	    availwdt -= colwdt;
	}
    }

    int nr = nrRows();
    if ( nr && setup_.fillcol_ )
    {
	int height = size->vNrPics();
	int availhgt = height - body_->horizontalHeader()->frameSize().height()
			 - 2*body_->frameWidth();

	int rowhgt =  availhgt / nr;
	const float fonthgt = (float)font()->height();

	const int minhgt = (int)(setup_.minrowhgt_ * fonthgt);
	const int maxhgt = (int)(setup_.maxrowhgt_ * fonthgt);

	if ( rowhgt < minhgt ) rowhgt = minhgt;
	if ( rowhgt > maxhgt ) rowhgt = maxhgt; 

	for ( int idx=0; idx < nr; idx ++ )
	{
	    if ( idx < nr-1 )
		setRowHeight( idx, rowhgt );
	    else
	    {
		int hgt = availhgt;
		if ( hgt < minhgt ) hgt = minhgt;
		if ( hgt > maxhgt ) hgt = maxhgt;

		setRowHeight( idx, hgt );
	    }
	    availhgt -= rowhgt;
	}
    }
}


void uiTable::clearTable()
{ body_->clearContents(); }

void uiTable::removeAllSelections()
{ body_->clearSelection(); }


bool uiTable::isSelected ( const RowCol& rc ) const
{
    const QItemSelectionModel* selmodel = body_->selectionModel();
    const QAbstractItemModel* model = selmodel ? selmodel->model() : 0;
    if ( !model )
	return false;

    QModelIndex idx = body_->rootIndex();
    idx = model->index( rc.row, rc.col, idx );
    return selmodel->isSelected( idx );
}


bool uiTable::isRowSelected( int row ) const
{
    QItemSelectionModel* model = body_->selectionModel();
    return model ? model->isRowSelected( row, body_->rootIndex() ) : false;
}


bool uiTable::isColumnSelected( int col ) const
{
    QItemSelectionModel* model = body_->selectionModel();
    return model ? model->isColumnSelected( col, body_->rootIndex() ) : false;
}


int uiTable::currentRow() const
{ return body_->currentRow(); }

int uiTable::currentCol() const
{ return body_->currentColumn(); }


void uiTable::setSelected( const RowCol& rc, bool yn )
{
    QTableWidgetItem* itm = body_->item( rc.row, rc.col );
    if ( itm )
	itm->setSelected( yn );
}


void  uiTable::selectRow( int row )
{ return body_->selectRow( row ); }

void  uiTable::selectColumn( int col )
{ return body_->selectColumn( col ); }

void uiTable::ensureCellVisible( const RowCol& rc )
{
    QTableWidgetItem* itm = body_->item( rc.row, rc.col );
    body_->scrollToItem( itm );
}


void uiTable::setCellGroup( const RowCol& rc, uiGroup* grp )
{
    if ( !grp ) return;
    body_->setCellObject( rc, grp->attachObj() );
}


uiGroup* uiTable::getCellGroup( const RowCol& rc ) const
{
    mDynamicCastGet(uiGroupObj*,grpobj,getCellObject(rc));
    return grpobj ? grpobj->group() : 0;
}


RowCol uiTable::getCell( uiGroup* grp )
{ return grp ? getCell( grp->attachObj() ) : RowCol(-1,-1); }


void uiTable::setCellObject( const RowCol& rc, uiObject* obj )
{ body_->setCellObject( rc, obj ); }

uiObject* uiTable::getCellObject( const RowCol& rc ) const
{ return body_->getCellObject( rc ); }

void uiTable::clearCellObject( const RowCol& rc )
{ body_->clearCellObject( rc ); }


RowCol uiTable::getCell( uiObject* obj )
{ return body_->getCell( obj ); }


const ObjectSet<uiTable::SelectionRange>& uiTable::selectedRanges() const
{
    deepErase( selranges_ );
    QList<QTableWidgetSelectionRange> qranges = body_->selectedRanges();
    for ( int idx=0; idx<qranges.size(); idx++ )
    {
	uiTable::SelectionRange* rg = new uiTable::SelectionRange;
	rg->firstrow_ = qranges[idx].topRow();
	rg->lastrow_ = qranges[idx].bottomRow();
	rg->firstcol_ = qranges[idx].leftColumn();
	rg->lastcol_ = qranges[idx].rightColumn();
	selranges_ += rg;
    }

    return selranges_;
}


uiTable::SelectionBehavior uiTable::getSelBehavior() const
{
    return body_->getSelBehavior();
}


int uiTable::maxSelectable() const
{
    return body_->maxSelectable();
}


void uiTable::selectItems( const TypeSet<RowCol>& rcs, bool yn )
{
    removeAllSelections();
    for ( int idx=0; idx<rcs.size(); idx++ )
    {
	if ( mIsUdf(rcs[idx].row) || mIsUdf(rcs[idx].col) )
	    continue;
	QTableWidgetItem* itm = body_->item( rcs[idx].row, rcs[idx].col );
	if ( !itm || (yn && itm->isSelected()) || (!yn && !itm->isSelected()) )
	    continue;
	itm->setSelected( yn );
    }
}
