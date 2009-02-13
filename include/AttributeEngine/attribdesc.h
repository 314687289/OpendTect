#ifndef attribdesc_h
#define attribdesc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdesc.h,v 1.46 2009-02-13 13:31:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "seistype.h"
#include "attribdescid.h"
#include "typeset.h"


namespace Attrib
{

class Desc;
class Param;
class DescSet;
class ValParam;

typedef void(*DescStatusUpdater)(Desc&);

mClass InputSpec
{
public:
    				InputSpec( const char* d, bool enabled_ )
				    : desc(d), enabled(enabled_)
				    , issteering(false)		{}

    const char*        		getDesc() const { return desc; }

    BufferString		desc;
    TypeSet<Seis::DataType>	forbiddenDts;
    bool			enabled;
    bool			issteering;

    bool			operator==(const InputSpec&) const;
};


/*!Description of an attribute in an Attrib::DescSet. Each attribute has
   a name (e.g. "Similarity"), a user reference (e.g. "My similarity"), and at
   least one output. In addition, it may have parameters and inputs. If it has
   multiple outputs, only one of the outputs are selected.

   The attrib name, the parameters and the selected output number together form
   a definition string that define what the attribute calculates.

   Each Desc has DescID that is unique within it's DescSet.
 */

mClass Desc
{ mRefCountImpl(Desc);
public:

				Desc(const Desc&);
				Desc(const char* attrname,
				     DescStatusUpdater updater=0);

    const char*			attribName() const;

    void			setDescSet(DescSet*);
    DescSet*			descSet() const;
    DescID			id() const;

    bool			getDefStr(BufferString&) const;
    bool			parseDefStr(const char*);

    const char*			userRef() const;
    void			setUserRef(const char*);

    int				nrOutputs() const;
    void			selectOutput(int);
    int				selectedOutput() const;

    Seis::DataType		dataType(int output=-1) const;
				/*!<\param output specifies which output is 
				     required, or -1 for the selected output.*/

    void			setSteering( bool yn )	{ issteering_=yn; }
    bool			isSteering() const	{ return issteering_; }

    void			setHidden( bool yn )	{ hidden_ = yn; }
    				/*!<If hidden, it won't show up in UI. */
    bool			isHidden() const	{ return hidden_; }
    				/*!<If hidden, it won't show up in UI. */

    bool			isStored() const;
    BufferString		getStoredID() const;

    void			setNeedProvInit( bool yn=true )
    				{ needprovinit_ = yn; }
    bool			needProvInit() const
    				{ return needprovinit_;}

    int				nrInputs() const;
    InputSpec&			inputSpec(int);
    const InputSpec&		inputSpec(int) const;
    bool        		setInput(int,const Desc*);
    Desc*			getInput(int);
    const Desc*			getInput(int) const;
    void			getDependencies(TypeSet<Attrib::DescID>&) const;
    				/*!<Generates list of attributes this attribute
				    is dependant on. */

    bool			is2D() const;
    void			set2D(bool);
    bool			is2DSet() const		{ return is2dset_; }

    enum SatisfyLevel		{ AllOk, Warning, Error };
    SatisfyLevel		isSatisfied() const;
				/*!< Checks whether all inputs are satisfied. */

    const BufferString&		errMsg() const;
    void			setErrMsg( const char* str )	{ errmsg_=str; }

    bool			isIdenticalTo(const Desc&,
	    				      bool cmpoutput=true) const;
    bool			isIdentifiedBy(const char*) const;
    DescID			inputId(int idx) const;

    				/* Interface to factory */
    void			addParam(Param*);
    				/*!< Pointer becomes mine */
    const Param*		getParam(const char* key) const;
    Param*			getParam(const char* key);
    const ValParam*		getValParam(const char* key) const;
    ValParam*			getValParam(const char* key);
    void			setParamEnabled(const char* key,bool yn=true);
    bool			isParamEnabled(const char* key) const;
    void			setParamRequired(const char* key,bool yn=true);
    bool			isParamRequired(const char* key) const;

    void			updateParams();
    void			changeStoredID(const char*);

    void			addInput(const InputSpec&);
    bool			removeInput(int idx);
    void			removeOutputs();
    void			addOutputDataType(Seis::DataType);
    void			setNrOutputs(Seis::DataType,int);
    void			addOutputDataTypeSameAs(int);
    void			changeOutputDataType(int,Seis::DataType);

    static bool			getAttribName(const char* defstr,BufferString&);
    static bool			getParamString(const char* defstr,
	    				       const char* key,BufferString&);

    Desc*			getStoredInput() const;
    
    static const char*		sKeyInlDipComp();
    static const char*		sKeyCrlDipComp();
    
protected:

    bool			setInput_(int,Desc*);
    Param*			findParam(const char* key);

    void			getKeysVals(const char* defstr,
	    			    BufferStringSet& keys,
				    BufferStringSet& vals);
				/*!<Fills \akeys and \avals with pairs of
				    parameters from the defstr. */

    TypeSet<Seis::DataType>	outputtypes_;
    TypeSet<int>		outputtypelinks_;
    bool			issteering_;
    bool			hidden_;
    bool			needprovinit_;
    mutable bool 		is2d_;
    mutable bool 		is2ddetected_;
    bool 			is2dset_;

    TypeSet<InputSpec>		inputspecs_;
    ObjectSet<Desc>		inputs_;

    BufferString		attribname_;
    ObjectSet<Param>		params_;

    BufferString        	userref_;
    int				seloutput_;
    DescSet*			descset_;

    DescStatusUpdater		statusupdater_;
    BufferString		errmsg_;
};

}; // namespace Attrib

#define mGetInt( var, varstring ) \
{\
    var = desc.getValParam(varstring)->getIntValue(0); \
    if ( mIsUdf(var) )\
        var = desc.getValParam(varstring)->getDefaultIntValue(0);\
}

#define mGetFloat( var, varstring ) \
{\
    var = desc.getValParam(varstring)->getfValue(0); \
    if ( mIsUdf(var) )\
        var = desc.getValParam(varstring)->getDefaultfValue(0);\
}

#define mGetBool( var, varstring ) \
{\
    Attrib::ValParam* valparam##var = \
            const_cast<Attrib::ValParam*>(desc.getValParam(varstring));\
    mDynamicCastGet(Attrib::BoolParam*,boolparam##var,valparam##var);\
    if ( boolparam##var ) \
        var = boolparam##var->isSet() ? boolparam##var->getBoolValue(0)\
				      : boolparam##var->getDefaultBoolValue(0);\
}

#define mGetEnum( var, varstring ) \
{\
    Attrib::ValParam* valparam##var = \
            const_cast<Attrib::ValParam*>(desc.getValParam(varstring));\
    mDynamicCastGet(Attrib::EnumParam*,enumparam##var,valparam##var);\
    if ( enumparam##var ) \
        var = enumparam##var->isSet() ? enumparam##var->getIntValue(0)\
				      : enumparam##var->getDefaultIntValue(0);\
}

#define mGetString( var, varstring ) \
{\
    var = desc.getValParam(varstring)->getStringValue(0); \
    if ( !strcmp( var, "" ) )\
        var = desc.getValParam(varstring)->getDefaultStringValue(0); \
}
    
#define mGetBinID( var, varstring ) \
{\
    var.inl = desc.getValParam(varstring)->getIntValue(0); \
    var.crl = desc.getValParam(varstring)->getIntValue(1); \
    if ( mIsUdf(var.inl) || mIsUdf(var.crl) )\
    {\
	Attrib::ValParam* valparam##var = \
	      const_cast<Attrib::ValParam*>(desc.getValParam(varstring));\
	mDynamicCastGet(Attrib::BinIDParam*,binidparam##var,valparam##var);\
	if ( binidparam##var ) \
	    var = binidparam##var->getDefaultBinIDValue();\
    }\
    if ( desc.descSet()->is2D() ) \
    	var.inl = 0; \
}

#define mGetFloatInterval( var, varstring ) \
{\
    var.start = desc.getValParam(varstring)->getfValue(0); \
    var.stop = desc.getValParam(varstring)->getfValue(1); \
    if ( mIsUdf(var.start) || mIsUdf(var.stop) )\
    {\
	Attrib::ValParam* valparam##var = \
	      const_cast<Attrib::ValParam*>(desc.getValParam(varstring));\
	mDynamicCastGet(Attrib::ZGateParam*,gateparam##var,valparam##var);\
	if ( gateparam##var ) \
	    var = gateparam##var->getDefaultGateValue();\
    }\
}

#endif


