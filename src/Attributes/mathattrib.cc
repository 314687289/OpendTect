/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "mathattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "mathexpression.h"
#include "separstr.h"

static const char* specvararr[] = { "DZ", "Inl", "Crl", 0 };
;

namespace Attrib
{
mAttrDefCreateInstance(Math)
}; // namespace Attrib


void Attrib::Math::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addParam( new StringParam(expressionStr()) );

    DoubleParam cst( cstStr() );
    ParamGroup<DoubleParam>* cstset =
			new ParamGroup<DoubleParam>( 0, cstStr(), cst );
    desc->addParam( cstset );

    desc->addParam( new DoubleParam(recstartStr(), 0, false) );
    desc->addParam( new StringParam(recstartvalsStr(), 0, false) );

    FloatParam* recstartpos = new FloatParam( recstartposStr() );
    recstartpos->setDefaultValue( 0 );
    recstartpos->setRequired( false );
    desc->addParam( recstartpos );

    desc->addInput( InputSpec("Data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


void Attrib::Math::updateDesc( Desc& desc )
{
    ValParam* expr = desc.getValParam( expressionStr() );
    if ( !expr ) return;

    ::Math::ExpressionParser mep( expr->getStringValue() );
    PtrMan< ::Math::Expression > formula = mep.parse();
    if ( !formula ) return;

    int nrconsts = 0;
    int nrvariables = 0;
    BufferStringSet varnms;
    for ( int idx=0; idx<formula->nrUniqueVarNames(); idx++ )
    {
	const char* uvarnm = formula->uniqueVarName(idx);
	const ::Math::Expression::VarType vtyp
			= ::Math::ExpressionParser::varTypeOf(uvarnm);
	switch ( vtyp )
	{
	    case ::Math::Expression::Variable :
	    {
		if ( !getSpecVars().isPresent( uvarnm ) )
		{
		    nrvariables++;
		    varnms.add( uvarnm );
		}
		break;
	    }
	    case ::Math::Expression::Constant :
		nrconsts++;
		break;
	    default:
		break;
	}
    }

    if ( desc.nrInputs() != nrvariables )
    {
	while ( desc.nrInputs() )
	    desc.removeInput(0);

	for ( int idx=0; idx<nrvariables; idx++ )
	    desc.addInput( InputSpec( varnms.get( idx ), true ) );
    }

    desc.setParamEnabled( cstStr(), nrconsts );

    bool isrec = formula->isRecursive();
    desc.setParamEnabled( recstartStr(), isrec );
    desc.setParamEnabled( recstartvalsStr(), isrec );
    desc.setParamEnabled( recstartposStr(), isrec );
}


const BufferStringSet& Attrib::Math::getSpecVars()
{
    mDefineStaticLocalObject( const BufferStringSet, res, (specvararr) );
    return res;
}


Attrib::Math::Math( Desc& dsc )
    : Provider( dsc )
    , desintv_( Interval<float>(0,0) )
    , reqintv_( Interval<int>(0,0) )
    , recstartpos_( 0 )
    , maxshift_( 0 )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    ValParam* expr = dsc.getValParam( expressionStr() );
    if ( !expr ) return;

    ::Math::ExpressionParser mep( expr->getStringValue() );
    expression_ = mep.parse();
    errmsg_ += mep.errMsg();
    if ( mep.errMsg() ) return;

    mDescGetParamGroup(DoubleParam,cstset,dsc,cstStr())
    for ( int idx=0; idx<cstset->size(); idx++ )
    {
	const ValParam& param = (ValParam&)(*cstset)[idx];
	csts_ += param.getdValue();
    }

    if ( expression_->isRecursive() )
    {
	SeparString recstartstr;
	mGetString( recstartstr, recstartvalsStr() );
	if ( recstartstr.isEmpty() )
	{
	    //backward compatibility v3.3 and previous
	    double recstartval = mUdf(double);
	    mGetDouble( recstartval, recstartStr() );
	    if ( !mIsUdf(recstartval) ) recstartvals_ += recstartval;
	}
	else
	{
	    const int nrvals = recstartstr.size();
	    for ( int idx=0; idx<nrvals; idx++ )
	    {
		double val = toDouble( recstartstr[idx] );
		if ( mIsUdf(val) )
		    break;
		recstartvals_ += val;
	    }
	}

	mGetFloat( recstartpos_, recstartposStr() );
	desintv_.start = -1000;	//ensure we get the entire trace beginning
    }

    setUpVarsSets();
}


bool Attrib::Math::allowParallelComputation() const
{
    return !expression_->isRecursive();
}


bool Attrib::Math::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Attrib::Math::getInputData( const BinID& relpos, int zintv )
{
    int nrinputs = expression_->nrUniqueVarNames() -
				   ( csts_.size() + specstable_.size() );
    while ( inputdata_.size() < nrinputs )
    {
	inputdata_ += 0;
	inputidxs_ += -1;
    }

    for ( int varidx=0; varidx<nrinputs; varidx++ )
    {
	const DataHolder* data = inputs_[varidx]->getData( relpos, zintv );
	if ( !data ) return false;

	inputdata_.replace( varidx, data );
	inputidxs_[varidx] = getDataIndex( varidx );
    }

    return true;
}


bool Attrib::Math::computeData( const DataHolder& output, const BinID& relpos,
			int z0, int nrsamples, int threadid ) const
{
    PtrMan< ::Math::Expression > mathobj
		= expression_ ? expression_->clone() : 0;
    if ( !mathobj ) return false;

    const bool isrec = expression_->isRecursive();
    const int nrxvars = varstable_.size();
    const int nrcstvars = cststable_.size();
    const int nrspecvars = specstable_.size();
    const int nrvar = mathobj->nrVariables();
    if ( (nrxvars + nrcstvars + nrspecvars) != nrvar )
	return false;

    const int recstartidx = mNINT32( recstartpos_/refstep_ );

    //in case first sample is undef prevent result=undef
    //on whole trace for recursive formulas
    bool hasudf = isrec ? true : false;

    //exceptional case: recstartpos_>z0
    if ( isrec && recstartidx>z0 )
    {
	for ( int idx=z0; idx<recstartidx; idx++ )
	{
	    if ( idx >= z0+nrsamples )
		return true;

	    setOutputValue( output, 0, idx-z0, z0,
			    mCast(float,recstartvals_[0]) );
	}
    }

    const int loopstartidx = isrec ? recstartidx : z0;
    const int loopstopidx = z0 + nrsamples;

    //A temp DataHolder is needed for recursive formulas
    const int tmpholdersz = loopstopidx - recstartidx;
    DataHolder* tmpholder = isrec ? new DataHolder(recstartidx, tmpholdersz): 0;
    if ( tmpholder ) tmpholder->add();

    for ( int idx=loopstartidx; idx<loopstopidx; idx++ )
    {
	const int sampidx = idx - loopstartidx;
	if ( isrec && sampidx == 0 )
	{
	    setOutputValue( *tmpholder, 0, sampidx, recstartidx,
			    mCast(float,recstartvals_[0]) );
	    if ( idx == z0 || recstartidx>z0 )
		setOutputValue( output, 0, idx-z0, z0,
				mCast(float,recstartvals_[0]) );

	    continue;
	}

	for ( int xvaridx=0; xvaridx<nrxvars; xvaridx++ )
	{
	    const int variableidx = varstable_[xvaridx].varidx_;
	    const int inpidx = varstable_[xvaridx].inputidx_;
	    const int shift = varstable_[xvaridx].shift_;
	    const DataHolder* inpdata = inpidx == -1 ? tmpholder
						     : inputdata_[inpidx];
	    const int refdhidx = inpidx == -1 ? recstartidx : 0;
	    const int inpsampidx = inpidx == -1 ? sampidx : idx;
	    int compidx = inpidx == -1 ? 0 : inputidxs_[inpidx];
	    double val = inpidx==-1 && shift+idx-loopstartidx<0
		? recstartvals_[shift-maxshift_]
		: mCast( double, getInputValue( *inpdata, compidx,
						inpsampidx+shift, refdhidx ) );

	    //in case first samp is undef prevent result=undef
	    //on whole trace for recursive formulas
	    if ( inpidx == -1 && mIsUdf( val ) && hasudf )
		val = recstartvals_[shift-maxshift_];

	    mathobj->setVariableValue( variableidx, val );
	}
	for ( int cstidx=0; cstidx<nrcstvars; cstidx++ )
	    mathobj->setVariableValue( cststable_[cstidx].fexpvaridx_,
				       csts_[cstidx] );

	for ( int specidx=0; specidx<nrspecvars; specidx++ )
	{
	    double val = mUdf(double);
	    switch ( specstable_[specidx].specidx_ )
	    {
		case 0 :	val = mCast( double, refstep_ ); break;
		case 1 :	val = mCast( double, currentbid_.inl() ); break;
		case 2 :	val = mCast( double, currentbid_.crl() ); break;
	    }
	    mathobj->setVariableValue( specstable_[specidx].fexpvaridx_, val );
	}

	const float result = mCast( float, mathobj->getValue() );

	if ( hasudf && !mIsUdf( result ) )
	    hasudf = false;

	if ( tmpholder )
	    tmpholder->series(0)->setValue( sampidx, result );

	if ( idx >= z0 )
	    setOutputValue( output, 0, sampidx+loopstartidx-z0, z0, result );
    }

    return true;
}


void Attrib::Math::setUpVarsSets()
{
    int nrcsts = 0;
    int nrspecs = 0;
    for ( int idx=0; idx<expression_->nrVariables(); idx++ )
    {
	BufferString fvarexp = expression_->fullVariableExpression( idx );
	const ::Math::Expression::VarType vtyp
		= ::Math::ExpressionParser::varTypeOf(fvarexp);
	switch ( vtyp )
	{
	    case ::Math::Expression::Variable :
	    {
		int shift=0;
		const BufferString varnm =
			::Math::ExpressionParser::varNameOf( fvarexp, &shift );
		const int specidx = getSpecVars().indexOf(varnm);
		if ( specidx >=0 )
		{
		    nrspecs++;
		    specstable_ += SPECS( idx, specidx );
		}
		else
		{
		    const int indexvarnm =
				expression_->indexOfUnVarName( varnm.buf() );
		    int inputidx = -1;
		    if ( indexvarnm>=0 )
		    {
			const int firstoccurvnm =
				    expression_->firstOccurVarName( fvarexp);
			if ( firstoccurvnm == idx )
			    inputidx = indexvarnm - nrcsts - nrspecs;
			else
			{
			    for ( int vidx=0; vidx<varstable_.size(); vidx++ )
				if ( varstable_[vidx].varidx_ == firstoccurvnm )
				{
				    inputidx = varstable_[vidx].inputidx_;
				    break;
				}
			}
		    }

		    if ( inputidx<0 && shift<maxshift_ )
			maxshift_ = shift;
		    varstable_ += VAR( idx, inputidx, shift );
		}
		break;
	    }
	    case ::Math::Expression::Constant :
	    {
		int constidx = expression_->getConstIdx( idx );
		cststable_.add( CSTS( idx, constidx ) );
		nrcsts++;
		break;
	    }
	    case ::Math::Expression::Recursive :
	    {
		int shift=0;
		const BufferString varnm =
			::Math::ExpressionParser::varNameOf( fvarexp, &shift );
		varstable_ += VAR( idx, -1, shift );
	    }
	}
    }
    if ( expression_->isRecursive() )
	adjustVarSampReqs();

    while ( recstartvals_.size()< -maxshift_ )
	recstartvals_+= recstartvals_.size() ?
				recstartvals_[ recstartvals_.size()-1] : 0;
}


const Interval<int>* Attrib::Math::reqZSampMargin( int inp, int ) const
{
    //Trick: as call to this function is not multithreaded
    //we use a single address for reqintv_ which will be reset for every input
    const_cast< Interval<int>* >(&reqintv_)->set(0,0);
    bool found = false;
    for ( int idx=0; idx<varstable_.size(); idx++ )
	if ( varstable_[idx].inputidx_ == inp )
	{
	    found = true;
	    const_cast< Interval<int>* >
		(&reqintv_)->include( varstable_[idx].sampgate_ );
	}

    return found ? &reqintv_ : 0;
}


const Interval<float>* Attrib::Math::desZMargin( int inp, int ) const
{
    return &desintv_;
}


void Attrib::Math::adjustVarSampReqs()
{
    for ( int idx=0; idx<varstable_.size(); idx++ )
    {
	Interval<int> varsgate = varstable_[idx].sampgate_;
	varsgate.start - maxshift_ <= varsgate.stop
			? varstable_[idx].sampgate_.start -= maxshift_
			: varstable_[idx].sampgate_.start = varsgate.stop;
    }
}
