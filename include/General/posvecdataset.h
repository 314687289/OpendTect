#ifndef posvecdataset_h
#define posvecdataset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jan 2005
 Contents:	Set with data vectors on positions
 RCS:		$Id: posvecdataset.h,v 1.7 2005-07-27 09:23:35 cvsbert Exp $
________________________________________________________________________

-*/

#include "binidvalset.h"
#include "bufstringset.h"
class IOPar;
class DataColDef;



/*!\brief Data set consisting of data vectors

  Every data vector has an inline, crossline and a Z (which may be undefined).
  The "Z" column is automatically added.

*/

class PosVecDataSet
{
public:
    			PosVecDataSet(const char* nm=0);
    			PosVecDataSet(const PosVecDataSet&);
    virtual		~PosVecDataSet();
    PosVecDataSet&	operator =(const PosVecDataSet&);
    void		copyStructureFrom(const PosVecDataSet&);

    void		empty();
    void		add(DataColDef*);
    void		removeColumn(int); //!< "Z" col (idx=0) can't be removed
    enum OvwPolicy	{ Keep, OvwIfUdf, Ovw };
    			//!< During merge, which set is dominant?
    enum ColMatchPol	{ NameExact, RefExact, NameStart, RefStart };
    void		merge(const PosVecDataSet&,OvwPolicy pol=OvwIfUdf,
			      ColMatchPol cmp=NameExact);
    			//!< This is a rather intelligent method.

    BinIDValueSet&	data()			{ return data_; }
    const BinIDValueSet& data() const		{ return data_; }

    int			nrCols() const		{ return coldefs_.size(); }
    DataColDef&		colDef( int idx )	{ return *coldefs_[idx]; }
    const DataColDef&	colDef( int idx ) const	{ return *coldefs_[idx]; }

    const char*		name() const		{ return name_.buf(); }
    void		setName( const char* nm ) { name_ = nm; }

    IOPar&		pars()			{ return pars_; }
    const IOPar&	pars() const		{ return pars_; }

    bool		getFrom(const char*,BufferString& errmsg);
    bool		putTo(const char*,BufferString& errmsg,
	    			bool tabstyle) const;
    			//!< tabstyle -> for spreadsheet import (looses info)
    			//!< !tabstyle: dTect style (preserves all)
    static bool		getColNames(const char*,BufferStringSet& bss,
	    			    BufferString& errmsg,bool refs=false);

protected:

    BinIDValueSet	data_;
    ObjectSet<DataColDef> coldefs_;
    BufferString	name_;
    IOPar&		pars_;

    void		mergeColDefs(const PosVecDataSet&,ColMatchPol,int*);

};

#define mPosVecDataSetFileType "Positioned Vector Data"


#endif
