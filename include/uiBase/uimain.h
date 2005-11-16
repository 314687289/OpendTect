#ifndef uimain_H
#define uimain_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          03/12/1999
 RCS:           $Id: uimain.h,v 1.9 2005-11-16 15:24:24 cvsarend Exp $
________________________________________________________________________

-*/

#include "uigeom.h"
class uiMainWin;
class QApplication;
class uiFont;
class QWidget;


class uiMain
{
public:
			uiMain(int argc,char** argv);
private:
			uiMain(QApplication*);

    void 		init(QApplication*, int argc, char **argv);

public:

    virtual		~uiMain();

    virtual int		exec();	
    void 		exit(int retcode=0);

    void		setTopLevel(uiMainWin*);
    uiMainWin*		topLevel()			{ return mainobj; }
    void		setFont(const uiFont& font,bool passtochildren);    

    const uiFont*	font(); 

    static uiMain&	theMain();
    static void		setTopLevelCaption( const char* );

    static void		flushX();

    /*!  Processes pending events for maxtime milliseconds
     *   or until there are no more events to process, whichever is shorter.
     *   Only works after themain has been constructed.
     */
    static void		processEvents(int msec=3000);

    static uiSize	desktopSize();

protected:

    static uiMain*	themain;
    uiMainWin*		mainobj;

    static QApplication*  app;
    static const uiFont*  font_;

			//! necessary for uicMain coin inialisation
    virtual void	init( QWidget* mainwidget )             {}
};

#endif
