/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/

#include "uiattrsetman.h"

#include "uibutton.h"
#include "uiioobjsel.h"
#include "uitextedit.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "ioobjctxt.h"
#include "survinfo.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiAttrSetMan)


uiAttrSetMan::uiAttrSetMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(uiStrings::phrManage(tr("Attribute Sets")),
				     mNoDlgTitle,
				     mODHelpKey(mAttrSetManHelpID) )
			       .nrstatusflds(1).modal(false),
	           AttribDescSetTranslatorGroup::ioContext())
{
    createDefaultUI();
    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiAttrSetMan::~uiAttrSetMan()
{
}


static void addAttrNms( const Attrib::DescSet& attrset, BufferString& txt,
			bool stor )
{
    const int totnrdescs = attrset.nrDescs( true, true );
    BufferStringSet nms;
    for ( int idx=0; idx<totnrdescs; idx++ )
    {
	const Attrib::Desc& desc = *attrset.desc( idx );
	if ( !desc.isHidden() && stor == desc.isStored() )
	    nms.add( desc.userRef() );
    }

    txt.add( nms.getDispString(2) );
}


void uiAttrSetMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    uiString errmsg;
    Attrib::DescSet attrset(!SI().has3D());
    if (!AttribDescSetTranslator::retrieve(attrset, curioobj_, errmsg))
    {
	BufferString msg("Read error: '"); msg += errmsg.getFullString(); 
	msg += "'"; txt = msg;
    }
    else
    {
	if (!errmsg.isEmpty())
	    ErrMsg(errmsg.getFullString());

	const int nrattrs = attrset.nrDescs( false, false );
	const int nrwithstor = attrset.nrDescs( true, false );
	const int nrstor = nrwithstor - nrattrs;
	txt = "Type: "; txt += attrset.is2D() ? "2D" : "3D";
	if ( nrstor > 0 )
	{
	    txt += "\nInput"; txt += nrstor == 1 ? ": " : "s: ";
	    addAttrNms( attrset, txt, true );
	}
	if ( nrattrs < 1 )
	    txt += "\nNo attributes defined";
	else
	{
	    txt += "\nAttribute"; txt += nrattrs == 1 ? ": " : "s: ";
	    addAttrNms( attrset, txt, false );
	}
    }

    txt += "\n";
    txt += getFileInfo();
    setInfo(txt);
}
