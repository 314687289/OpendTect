#ifndef uitblimpexpdatasel_h
#define uitblimpexpdatasel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uitblimpexpdatasel.h,v 1.7 2007-01-08 17:11:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "multiid.h"
class uiGenInput;
class uiTableFmtDescFldsParSel;
namespace Table { class FormatDesc; }

/*!\brief Table-based data import selection

  This class is meant to accept data structures describing table import/export
  as defined in General/tabledef.h. Resulting FormatDesc's selections can be
  used by a descendent of Table::AscIO.

  For example, the Wavelet import dialog creates a Table::FormatDesc object
  using WaveletAscIO::getDesc(), this class lets the user fill the FormatDesc's
  selection_, after which WaveletAscIO creates the imported object.
 
 */

class uiTableImpDataSel : public uiGroup
{
public:

    				uiTableImpDataSel(uiParent*,Table::FormatDesc&);

    Table::FormatDesc&		desc()		{ return fd_; }
    const BufferString&		fmtName()	{ return fmtname_; }
    				//!< May not be correct: it's the last selected

    bool			commit();
    int				nrHdrLines() const; //!< '-1' = variable

    				// The following may be incorrect:
    bool			isstored_;
    BufferString		fmtname_;

protected:

    Table::FormatDesc&		fd_;

    uiGenInput*			hdrtypefld_;
    uiGenInput*			hdrlinesfld_;
    uiGenInput*			hdrtokfld_;
    uiTableFmtDescFldsParSel*	fmtdeffld_;
    friend class		uiTableFmtDescFldsParSel;

    void			typChg(CallBacker*);
    void			openFmt(CallBacker*);
};


#endif
