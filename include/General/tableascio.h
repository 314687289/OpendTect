#ifndef tableascio_h
#define tableascio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Nov 2006
 RCS:		$Id: tableascio.h,v 1.8 2008-12-25 11:44:29 cvsranojay Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "repos.h"
#include <iosfwd>
class IOPar;
class UnitOfMeasure;

namespace Table
{

class FormatDesc;
class ImportHandler;
class ExportHandler;
class Converter;
class FileFormatRepository;

FileFormatRepository& FFR();


/*!\brief Ascii I/O using Format Description.

  The idea is to create a subclass of AscIO which synthesises an object
  from the Selection of a Table::FormatDesc. Or, in the case of export,
  outputs the data according to the Selection object.
 
 */

mClass AscIO
{
public:

				AscIO( const FormatDesc& fd )
				    : fd_(fd)
				    , imphndlr_(0)
				    , exphndlr_(0)
				    , cnvrtr_(0) { units_.allowNull(true);}
    virtual			~AscIO();

    const FormatDesc&		desc() const		{ return fd_; }
    const char*			errMsg() const		{ return errmsg_.buf();}

protected:

    const FormatDesc&		fd_;
    mutable BufferString	errmsg_;
    BufferStringSet		vals_;
    ObjectSet<const UnitOfMeasure> units_;
    ImportHandler*		imphndlr_;
    ExportHandler*		exphndlr_;
    Converter*			cnvrtr_;

    friend class		AscIOImp_ExportHandler;
    friend class		AscIOExp_ImportHandler;

    void			emptyVals() const;
    void			addVal(const char*,const UnitOfMeasure*) const;
    bool			getHdrVals(std::istream&) const;
    int				getNextBodyVals(std::istream&) const;
    				//!< Executor convention
    bool			putHdrVals(std::ostream&) const;
    bool			putNextBodyVals(std::ostream&) const;

    const char*			text(int) const; // Never returns null
    int				getIntValue(int,int udf=mUdf(int)) const;
    float			getfValue(int,float udf=mUdf(float)) const;
    double			getdValue(int,double udf=mUdf(double)) const;
    				// For more, use Conv:: stuff

};


/*!\brief Holds system- and user-defined formats for different data types
  ('groups') */

mClass FileFormatRepository
{
public:

    void		getGroups(BufferStringSet&) const;
    void		getFormats(const char* grp,BufferStringSet&) const;

    const IOPar*	get(const char* grp,const char* nm) const;
    void		set(const char* grp,const char* nm,
	    		    IOPar*,Repos::Source);
			    //!< IOPar* will become mine; set to null to remove

    bool		write(Repos::Source) const;

protected:

    			FileFormatRepository();
    void		addFromFile(const char*,Repos::Source);
    const char*		grpNm(int) const;
    int			gtIdx(const char*,const char*) const;

    struct Entry
    {
			Entry( Repos::Source src, IOPar* iop )
			    : iopar_(iop), src_(src)	{}
			~Entry();

	IOPar*		iopar_;
	Repos::Source	src_;
    };

    ObjectSet<Entry>	entries_;

    friend FileFormatRepository& FFR();

};


}; // namespace Table


#endif
