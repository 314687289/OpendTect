#ifndef uiwellpropertyrefsel_h
#define uiwellpropertyrefsel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "multiid.h"
#include "propertyref.h"
#include "welllogset.h"
#include "uigroup.h"
#include "uiwellmod.h"

class ElasticPropSelection;
class PropertyRef;
class UnitOfMeasure;

class uiLabel;
class uiComboBox;
class uiCheckBox;
class uiButton;
class uiUnitSel;

namespace Well { class LogSet; }


mExpClass(uiWell) uiPropSelFromList : public uiGroup
{
public:
			uiPropSelFromList(uiParent*,const PropertyRef&,
					const PropertyRef* alternatepr=0);
			~uiPropSelFromList();

    void                setNames(const BufferStringSet& nms);

    void                set(const char* txt,bool alt,const UnitOfMeasure* u=0);
    void                setCurrent(const char*);
    void                setUOM(const UnitOfMeasure&);

    const char*         text() const;
    const UnitOfMeasure* uom() const;

    void                setUseAlternate(bool yn);
    bool                isUseAlternate() const;

    const PropertyRef&  propRef() const;
    const PropertyRef*  altPropRef() const { return altpropref_; }

    uiComboBox*   	typeFld() const         { return typefld_; }
    uiLabel*		getLabel() const	{ return typelbl_; }
    Notifier<uiPropSelFromList>	comboChg_;

protected:
    const PropertyRef&  propref_;
    const PropertyRef*  altpropref_;

    uiLabel*            typelbl_;
    uiComboBox*         typefld_;
    uiUnitSel*          unfld_;
    uiCheckBox*         checkboxfld_;

    void		updateSelCB(CallBacker*);
    void                switchPropCB(CallBacker*);
};


mExpClass(uiWell) uiWellPropSel : public uiGroup
{
public:

			uiWellPropSel(uiParent*,const PropertyRefSelection&);
    int			size() const	{ return propflds_.size(); }

    bool		setLogs(const Well::LogSet&);
    bool		setLog(const PropertyRef::StdType,const char*,
	    			bool check,const UnitOfMeasure*, int idx);
    bool		getLog(const PropertyRef::StdType,BufferString&,
	    			bool&, BufferString& uom, int idx) const;

    uiPropSelFromList*	getPropSelFromListByName(const BufferString&);
    uiPropSelFromList*	getPropSelFromListByIndex(int);
    virtual bool	isOK() const;
    void		setWellID( const MultiID& wid ) { wellid_ = wid; }

    MultiID		wellid_;

protected:
    void				initFlds();

    const PropertyRefSelection&  	proprefsel_;
    ObjectSet<uiPropSelFromList> 	propflds_;
    void		updateSelCB(CallBacker*);

    static const char*			sKeyPlsSel() { return "Please select"; }
};


mExpClass(uiWell) uiWellPropSelWithCreate : public uiWellPropSel
{
public:
			uiWellPropSelWithCreate(uiParent*,
				const PropertyRefSelection&);

    uiButton*		getButton( int idx )	{ return createbuts_[idx]; }

    Notifier<uiWellPropSel> logscreated; 

protected:

    ObjectSet<uiButton> createbuts_;
    void		createLogPushed(CallBacker*);

};



mExpClass(uiWell) uiWellElasticPropSel : public uiWellPropSel
{
public:
			uiWellElasticPropSel(uiParent*,bool withswaves=false);
			~uiWellElasticPropSel();

};


#endif

