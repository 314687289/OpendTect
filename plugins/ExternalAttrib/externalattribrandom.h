#ifndef externalattribrandom_h
#define externalattribrandom_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2008
 RCS:		$Id: externalattribrandom.h,v 1.4 2011-11-04 08:20:16 cvskris Exp $
________________________________________________________________________


-*/

#include "attribparam.h"
#include "externalattrib.h"
#include "multiid.h"
#include "menuhandler.h"
#include "uioddatatreeitem.h"


namespace ExternalAttrib
{


//Class that calculates the random values
class Random : public Attrib::ExtAttribCalc
{
public:
    static void			initClass();
    				Random();
    				~Random();

    static const char*		sAttribName()	{ return "Random"; }

    bool			setTargetSelSpec(const Attrib::SelSpec&);
    DataPack::ID		createAttrib(const CubeSampling&,DataPack::ID,
	    				     TaskRunner*);
    const Attrib::DataCubes*	createAttrib(const CubeSampling&,
	    				     const Attrib::DataCubes*);
    bool			createAttrib(ObjectSet<BinIDValueSet>&,
	    				     TaskRunner*);
    bool			createAttrib(const BinIDValueSet&, SeisTrcBuf&,
	    				     TaskRunner*);
    DataPack::ID		createAttrib(const CubeSampling&,
	    				     const LineKey&,TaskRunner*);

    bool			isIndexes() const;

protected:

    static Attrib::ExtAttribCalc* create(const Attrib::SelSpec&);
};


/* Class that manages the menus. One instance of this class resides in memory
   and makes sure that every time an object in the visualization is clicked,
   createMenu is called.
*/

class RandomManager : public CallBacker
{
public:
    		RandomManager();
		~RandomManager();

protected:

    void	createMenuCB(CallBacker*);
    void	handleMenuCB(CallBacker*);

    MenuItem	addrandomattribmnuitem_;
};


/* Class that holds the external attrib's tree-item. Can easily be complemented
   with meny handling by implementing more intelligent createMenu/handleMenu. */

class uiRandomTreeItem : public uiODDataTreeItem
{
public:
    static void		initClass();
    			uiRandomTreeItem(const char* parenttype);

    static const char*	sKeyDefinition() { return "Random"; }

protected:

    bool		anyButtonClick( uiListViewItem* );
    BufferString	createDisplayName() const;
    void		updateColumnText( int );

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

    static uiODDataTreeItem*	create( const Attrib::SelSpec& as,
				        const char* parenttype );
};

}; //namespace

#endif
