#ifndef uiattribpartserv_h
#define uiattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiattribpartserv.h,v 1.19 2006-12-14 14:30:51 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "attribdescid.h"
#include "position.h"
#include "multiid.h"
#include "menuhandler.h"
#include "timer.h"

namespace Attrib
{
    class DataCubes;
    class Data2DHolder;
    class Desc;
    class DescSet;
    class DescSetMan;
    class EngineMan;
    class SelInfo;
    class SelSpec;
};

class BinID;
class BinIDValueSet;
class BufferStringSet;
class CubeSampling;
class Executor;
class IOPar;
class NLACreationDesc;
class NLAModel;
class PosVecDataSet;
class SeisTrcBuf;
class SeisTrcInfo;
class uiAttribDescSetEd;
namespace Pick { class Set; }
template <class T> class Interval;
template <class T> class Array3D;


/*! \brief Service provider for application level - Attributes */

class uiAttribPartServer : public uiApplPartServer
{
public:
			uiAttribPartServer(uiApplService&);
			~uiAttribPartServer();

    const char*		name() const			{ return "Attributes"; }

    static const int	evDirectShowAttr;
    			//!< User requested direct redisplay of curAttrDesc()
    static const int	evNewAttrSet;
    			//!< FYI
    static const int	evAttrSetDlgClosed;
    			//!< AttributeSet window closes
    static const int	evEvalAttrInit;
    			//!< Initialization of evaluation dialog
    static const int	evEvalCalcAttr;
    			//!< User wants to evaluate current attribute
    static const int	evEvalShowSlice;
    			//!< Display slice
    static const int	evEvalStoreSlices;
    			//!< Store slices
    static const int	objNLAModel2D;
    			//!< Request current 2D NLAModel* via getObject()
    static const int	objNLAModel3D;
    			//!< Request current 3D NLAModel* via getObject()

    void		manageAttribSets();
    const Attrib::DescSet* curDescSet(bool) const;
    void		getDirectShowAttrSpec(Attrib::SelSpec&) const;
    bool		setSaved(bool) const;
    void		saveSet(bool);
    bool		editSet(bool);
    			//!< returns whether new AttribDescSet has been created
    bool		attrSetEditorActive() const	{ return attrsetdlg_; }
    void		updateSelSpec(Attrib::SelSpec&) const;

    bool		selectAttrib(Attrib::SelSpec&,const char*,bool);
    bool		setPickSetDirs(Pick::Set&,const NLAModel*);
    void		outputVol(MultiID&,bool);
    bool		replaceSet(const IOPar&,bool);
    bool		addToDescSet(const char*,bool);
    int			getSliceIdx() const		{ return sliceidx_; }
    void		getPossibleOutputs(bool,BufferStringSet&) const;

    void		setTargetSelSpec(const Attrib::SelSpec&);
    const TypeSet<Attrib::SelSpec>& getTargetSelSpecs() const
			    { return targetspecs_; }

    const Attrib::DataCubes* createOutput(const CubeSampling&,
				          const Attrib::DataCubes* prevslcs=0);
    bool		createOutput(ObjectSet<BinIDValueSet>&);
    bool		createOutput(const BinIDValueSet&,SeisTrcBuf&);
    bool		isDataAngles(bool) const;
			/*!<\returns true if the target data is an
				     angle, i.e. -PI==PI. */
    bool		isDataClassified(const Array3D<float>&) const;

    Attrib::DescID	createStored2DAttrib(const MultiID& lineset,
	    				     const char* attribname);
    
    bool		create2DOutput(const CubeSampling&, const char* linekey,
	    			       Attrib::Data2DHolder& );

    bool		extractData(const NLACreationDesc&,
				    const ObjectSet<BinIDValueSet>&,
				    ObjectSet<PosVecDataSet>&);
    bool		createAttributeSet(const BufferStringSet&,
	    				   Attrib::DescSet*);

    const NLAModel*	getNLAModel(bool) const;
    void		setNLAName( const char* nm )	{ nlaname_ = nm; }

    MenuItem*         	storedAttribMenuItem(const Attrib::SelSpec&,bool);
    MenuItem*         	calcAttribMenuItem(const Attrib::SelSpec&,bool);
    MenuItem*         	nlaAttribMenuItem(const Attrib::SelSpec&,bool);
    MenuItem*         	depthdomainAttribMenuItem(const Attrib::SelSpec&,
	    					  const char* key,bool);

    bool		handleAttribSubMenu(int mnuid,Attrib::SelSpec&) const;

    void		setEvaluateInfo(bool ae,bool as)
			{ alloweval_=ae; allowevalstor_=as; }

    void		fillPar(IOPar&,bool) const;
    void		usePar(const IOPar&,bool);

protected:

    MenuItem            stored2dmnuitem_;
    MenuItem            stored3dmnuitem_;
    MenuItem            calc2dmnuitem_;
    MenuItem            calc3dmnuitem_;
    MenuItem            nla2dmnuitem_;
    MenuItem            nla3dmnuitem_;
    MenuItem            depthdomain2dmnuitem_;
    MenuItem            depthdomain3dmnuitem_;

    Attrib::DescSetMan*	adsman2d_;
    Attrib::DescSetMan*	adsman3d_;
    const Attrib::Desc*	dirshwattrdesc_;
    uiAttribDescSetEd*	attrsetdlg_;
    Timer		attrsetclosetim_;

    Attrib::EngineMan*	createEngMan(const CubeSampling* cs=0,
	    			     const char* lk=0);

    void		directShowAttr(CallBacker*);

    void		showEvalDlg(CallBacker*);
    void		calcEvalAttrs(CallBacker*);
    void		showSliceCB(CallBacker*);
    void		evalDlgClosed(CallBacker*);

    void		attrsetDlgClosed(CallBacker*);
    void		attrsetDlgCloseTimTick(CallBacker*);

    Attrib::DescSetMan* getAdsMan(bool) const;
    Attrib::DescSetMan* getAdsMan(bool);

    BufferStringSet	get2DStoredItems(const Attrib::SelInfo&) const;
    void		insert2DStoredItems(const BufferStringSet&,int,int,bool,
	    				    MenuItem*,const Attrib::SelSpec&);
    void		insertNumerousItems(const BufferStringSet&,
	    				    const Attrib::SelSpec&,bool,bool);

    static const char*	attridstr_;
    BufferString	nlaname_;

    bool		alloweval_;
    bool		allowevalstor_;
    int			sliceidx_;
    Attrib::DescSet*	evalset;
    TypeSet<Attrib::SelSpec> targetspecs_;
};


/*!\mainpage Attribute User Interface

 This part server's main task is handling the attribute set editor. Other
 services are selection and volume output.

 The Attribute set editor is a pretty complex piece of user interface. The left
 and top part of the window are fixed. They handle the 'common' things in
 attribute set editing. The right part is defined via the uiAttribFactory .

 The proble that was facing us was that we needed a user interface that could
 be dynamically extended. Further more,much of the needed functionality is
 common to all attributes. Thus, we defined:

 - A base class for all attribute editors (uiAttrDescEd)
 - A factory of uiAttrDescEdCreater, creating uiAttrDescEd instances from
   the name of the attribute

 The uiAttrDescEd itself already has a lot of implemented functionality,
 leaving only things specific for that particular attribute to be defined.
 Once such a subclass is fully defined, a uiAttrDescEdCreater instance must be
 added to the factory to make it active.

 To see how such a new attribute can be created aswell as a user interface for
 it, take a look at the Coherency example in the programmer documentation on
 plugins.

*/


#endif
