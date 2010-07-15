#ifndef segydirect_h
#define segydirect_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segydirectdef.h,v 1.17 2010-07-15 18:43:36 cvskris Exp $
________________________________________________________________________

-*/

#include "segyfiledata.h"
#include "bufstringset.h"
#include "executor.h"

class StreamData;
class IOObj;
namespace Seis { class PosIndexer; }
namespace PosInfo { class CubeData; class Line2DData; }


namespace SEGY {

class FileSpec;
class Scanner;
class FileDataSet;
class PosKeyList;


mClass DirectDef
{
public:

    			DirectDef();			//!< Create empty
    			DirectDef(const char*);	//!< Read from file
			~DirectDef();


			//Functions to read/query
    bool		readFromFile(const char*);
    const IOPar*	segyPars() const;
    FileDataSet::TrcIdx	find(const Seis::PosKey&,bool chkoffs) const;
    FixedString		fileName(int idx) const;

    			//Functions to write
    void		setData(FileDataSet&);
    bool		writeHeadersToFile(const char*);
    			/*!<Write the headers. After calling, the fds should
			    be dumped into the stream. */
    std::ostream*	getOutputStream();
    bool		writeFootersToFile();
    			/*!<After fds has been dumped, write the 
			    remainder of the file */

    const char*		errMsg() const		{ return errmsg_.str(); }

    static const char*	sKeyDirectDef;
    static const char*	sKeyFileType;
    static const char*	sKeyNrFiles;
    static const char*	sKeyInt64DataChar;
    static const char*	sKeyInt32DataChar;
    static const char*	sKeyFloatDataChar;

    static const char*	get2DFileName(const char*,const char*);

    const PosInfo::CubeData&	cubeData() const { return cubedata_; }
    const PosInfo::Line2DData&	lineData() const { return linedata_; }


protected:
    void		getPosData(PosInfo::CubeData&) const;
    void		getPosData(PosInfo::Line2DData&) const;


    PosInfo::CubeData&	 cubedata_;
    PosInfo::Line2DData& linedata_;

    const FileDataSet*	fds_;
    FileDataSet*	myfds_;
    SEGY::PosKeyList*	keylist_;
    Seis::PosIndexer*	indexer_;

    mutable BufferString errmsg_;

    StreamData*		outstreamdata_;
    od_int64		offsetstart_;
    od_int64		datastart_;
    od_int64		textparstart_;
    od_int64		cubedatastart_;
    od_int64		indexstart_;
};


/*!Scans a pre-stack file and creates an index file that can be read by OD. */
mClass PreStackIndexer : public Executor
{
public:
    			PreStackIndexer(const MultiID& mid,
				const char* linename,
				const FileSpec&,
				bool is2d,const IOPar&);
    			~PreStackIndexer();

    int                 nextStep();

    const char*         message() const;
    od_int64            nrDone() const;
    od_int64            totalNr() const;
    const char*         nrDoneText() const;

    const Scanner*	scanner() const { return scanner_; }

protected:

    IOObj*		ioobj_;
    BufferString	linename_;

    Scanner*		scanner_;
    BufferString	msg_;
    DirectDef*		directdef_;
    
};


}; //Namespace

#endif
