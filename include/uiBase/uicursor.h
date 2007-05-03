#ifndef uicursor_h
#define uicursor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.h,v 1.8 2007-05-03 18:11:44 cvskris Exp $
________________________________________________________________________

-*/

class ioPixmap;
class QCursor;

class uiCursor 
{
public:

		    /*! This enum type defines the various cursors that can be
		        used.

			\value Arrow  standard arrow cursor
			\value UpArrow  upwards arrow
			\value Cross  crosshair
			\value Wait  hourglass/watch
			\value Ibeam  ibeam/text entry
			\value SizeVer  vertical resize
			\value SizeHor  horizontal resize
			\value SizeFDiag  diagonal resize (\)
			\value SizeBDiag  diagonal resize (/)
			\value SizeAll  all directions resize
			\value Blank  blank/invisible cursor
			\value SplitV  vertical splitting
			\value SplitH  horizontal splitting
			\value PointingHand  a pointing hand
			\value Forbidden  a slashed circle
			\value WhatsThis  an arrow with a question mark
			\value Bitmap

			Arrow is the default for widgets in a normal state.
		    */
    enum Shape	{ Arrow, UpArrow, Cross, Wait, Ibeam,
		  SizeVer, SizeHor, SizeBDiag, SizeFDiag, SizeAll,
		  Blank, SplitV, SplitH, PointingHand, Forbidden,
		  WhatsThis, Last = WhatsThis, Bitmap = 24
		};

    		uiCursor(Shape=Arrow);
    		uiCursor(const ioPixmap&,int hotx=-1,int hoty=-1);
		~uiCursor();

    void	setShape(Shape);
    void	setPixmap(const ioPixmap&,int hotx=-1, int hoty=-1);

		/*! \brief Sets another cursor for current application

		    example:

		    \code
			uiCursor::setOverride( uiCursor::Wait );
			calculateHugeMandelbrot();              // lunch time...
			uiCursor::restoreOverride();
		    \endcode    

		    Application cursors are stored on an internal stack.
		    setOverride() pushes the cursor onto the stack, and
		    restoreOverride() pops the active cursor off the stack.
		    Every setOverride() must eventually be followed by a
		    corresponding restoreOverride(), otherwise the stack will
		    never be emptied.  

		    If replace is true, the new cursor will replace the last
		    overridecw cursor (the stack keeps its depth). If replace is
		    FALSE, the new cursor is pushed onto the top of the stack.
		*/
    static void	setOverride(const uiCursor&,bool replace=false);
    static void	restoreOverride();

    const QCursor& qcursor() const;

protected:

    QCursor&	qcursor_;
};


/*! Class to automaticly change cursor, and change it back automaticly when
    class is running out of scope.
*/

class uiCursorChanger
{
public:
		uiCursorChanger(const uiCursor& c)
			{ uiCursor::setOverride(c); }
		uiCursorChanger(uiCursor::Shape cs)
			{ uiCursor::setOverride(uiCursor(cs)); }
		~uiCursorChanger() { uiCursor::restoreOverride(); }
};


#endif
