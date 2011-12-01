#ifndef welltied2tmodelmanager_h
#define welltied2tmodelmanager_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltied2tmodelmanager.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "namedobj.h"
#include "welltiegeocalculator.h"

class MultiID;
namespace Well
{
    class Data;
    class D2TModel;
}

/*!\brief Manages the D2T models used during TWTS.*/

namespace WellTie
{

class Data;
class DataWriter;

mClass D2TModelMgr
{
public:
			D2TModelMgr(Well::Data&,DataWriter&,const Data&);
			~D2TModelMgr();

    bool 		undo();
    bool 		cancel();

    bool      		updateFromWD();
    bool      		commitToWD();

    void		setWD( Well::Data* wd ) 	{ wd_ = wd; }
    void 		shiftModel(float);
    void 		setAsCurrent(Well::D2TModel*);
    void		setFromData(float* dah,float* time,int sz);

    const Data& 	getData() const { return data_; }

protected:

    Well::Data* 	wd_;

    Well::D2TModel* 	d2T();
    Well::D2TModel* 	prvd2t_;
    Well::D2TModel* 	orgd2t_;

    GeoCalculator	calc_;
    DataWriter&		datawriter_;
    const Data&		data_;

    void		ensureValid(Well::D2TModel&);
};

}; //namespace WellTie

#endif
