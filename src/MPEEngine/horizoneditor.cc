/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: horizoneditor.cc,v 1.2 2006-04-27 15:53:13 cvskris Exp $
________________________________________________________________________

-*/

#include "horizoneditor.h"
#include "geeditorimpl.h"
#include "binidsurface.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emhistory.h"
#include "mpeengine.h"

#define mSinus		0
#define mPyramid	1
#define mBox		2

namespace MPE
{

HorizonEditor::HorizonEditor( EM::Horizon& horizon )
    : ObjectEditor(horizon)
    , editarea( 5, 5 )
    , horbox( false )
    , vertstyle( 0 )
{
    emobject.notifier.notify( mCB( this, HorizonEditor, emChangeCB) );
    vertstylenames.add("Sinus");
    vertstylenames.add("Pyramid");
    vertstylenames.add("Box");
}

HorizonEditor::~HorizonEditor()
{
    emobject.notifier.remove( mCB( this, HorizonEditor, emChangeCB) );
}


ObjectEditor* HorizonEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::Horizon*,horizon,&emobj);
    return horizon ? new HorizonEditor( *horizon ) : 0;
}


void HorizonEditor::initClass()
{
    engine().addEditorFactory(
	    new EditorFactory( EM::Horizon::typeStr(), create ) );
}


Geometry::ElementEditor* HorizonEditor::createEditor( const EM::SectionID& sid )
{
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::BinIDSurface*,surface,ge);
    if ( !surface ) return 0;
    
    return new Geometry::BinIDElementEditor( 
		*const_cast<Geometry::BinIDSurface*>(surface) );
}


void HorizonEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    // TODO: display seeds and duplicate every 10th inline or so.
    ids.erase();

    const TypeSet<EM::PosID>* pids =
	emobject.getPosAttribList( EM::EMObject::sPermanentControlNode );

    if ( pids ) ids = *pids;

    pids = emobject.getPosAttribList( EM::EMObject::sTemporaryControlNode );
    if ( pids ) ids.createUnion( *pids );
}


bool HorizonEditor::addEditID( const EM::PosID& pid )
{
    emobject.setPosAttrib( pid, EM::EMObject::sPermanentControlNode, true,true);
    EM::EMM().history().setCurEventAsUserInteraction();
    return true;
}


bool HorizonEditor::removeEditID( const EM::PosID& pid )
{
    emobject.setPosAttrib( pid, EM::EMObject::sPermanentControlNode,false,true);
    return true;
}


const BufferStringSet* HorizonEditor::getAlongMovingStyleNames() const
{ return &vertstylenames;}


void HorizonEditor::getAlongMovingNodes( const EM::PosID&,
					 TypeSet<EM::PosID>& nodes,
					 TypeSet<float>* factors ) const
{
    nodes.erase();
    if ( factors ) factors->erase();

    if ( movingnode.objectID()==-1 )
	return;

    mDynamicCastGet( const EM::Horizon*, horizon, &emObject() );
    if ( !horizon )
	return;

    const EM::SectionID sectionid = movingnode.sectionID();
    const RowCol rc = movingnode.subID();
    const RowCol step = horizon->geometry().step();

    for ( int ridx=-editarea.row; ridx<=editarea.row; ridx++ )
    {
	for ( int cidx=-editarea.col; cidx<=editarea.col; cidx++ )
	{
	    float effect = 1;
	    if ( !horbox )
	    {
		const float relrow = (float)ridx/editarea.row;
		const float relcol = (float)cidx/editarea.col;
		const float curradius = sqrt(relrow*relrow+relcol*relcol);
		if ( curradius>1 ) 
		    continue;

		effect = 1.0-curradius;
	    }
	    else
	    {
		const float length = sqrt(ridx*ridx+cidx*cidx);

		if ( ridx )
		{
		    const float rowfactor = fabs(ridx)/(editarea.row+1);
		    const float colfactor = fabs(cidx)/rowfactor;

		    const float totallength =
			sqrt(editarea.row*editarea.row+colfactor*colfactor);

		    effect = 1-length/totallength;
		}

		if ( cidx )
		{
		    const float colfactor = fabs(cidx)/(editarea.col+1);
		    const float rowfactor = fabs(ridx)/colfactor;

		    const float totallength =
			sqrt(editarea.col*editarea.col+rowfactor*rowfactor);

		    const float neweffect = 1-length/totallength;
		    effect = mMIN(neweffect,effect);
		}
	    }

	    if ( vertstyle==mSinus )
		effect = sin( effect*M_PI_2 );
	    else if ( vertstyle==mBox )
		effect = effect ? 1 : 0;
	    
	    const RowCol currc =  rc + step*RowCol(ridx,cidx);
	    if ( horizon->isDefined(sectionid,currc.getSerialized()) )
	    {
		nodes += EM::PosID( horizon->id(), sectionid,
					     currc.getSerialized() );
		if ( factors ) (*factors) += effect;
	    }
	}
    }
}


void HorizonEditor::emChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if (  cbdata.event==EM::EMObjectCallbackData::AttribChange )
    {
	if ( cbdata.attrib==EM::EMObject::sPermanentControlNode ||
	     cbdata.attrib==EM::EMObject::sTemporaryControlNode )
	{
	    editpositionchange.trigger();
	}
    }
}



}; // namespace MPE
