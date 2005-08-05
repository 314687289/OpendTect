/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Mar 2000
 RCS:           $Id: mathexpression.cc,v 1.35 2005-08-05 21:09:43 cvskris Exp $
________________________________________________________________________

-*/

#include "mathexpressionimpl.h"
#include "ctype.h"
#include "ptrman.h"

#define absolute( str, idx, inabs) \
    if ( str[idx]=='|' && !(str[idx+1]=='|' || (idx && str[idx-1]=='|') ) ) \
	inabs=(inabs+1)%2;

#ifndef M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif


const char* MathExpression::getVariableStr( int var ) const
{
    if ( var>=getNrVariables() ) return 0;

    int input = (*variableobj[var])[0];
    int v = (*variablenr[var])[0];
    return inputs[input]->getVariableStr( v );
}


void MathExpression::setVariable( int var, float val )
{
    if ( var>=getNrVariables() || var<0 ) return;

    for ( int idx=0; idx<variableobj[var]->size(); idx++ )
    {
	int input = (*variableobj[var])[idx];
	int v = (*variablenr[var])[idx];

	inputs[input]->setVariable( v, val );
    }
}


bool MathExpression::setInput( int inp, MathExpression* obj )
{
    if ( inp>=0 && inp<getNrInputs() )
    {
	if ( inputs[inp] ) return false;
	delete inputs.replace( inp, obj );

	for ( int idx=0; idx<obj->getNrVariables(); idx++ )
	{
	    const char* str = obj->getVariableStr(idx);

	    bool found=false;

	    for ( int idy=0; idy<getNrVariables(); idy++ )
	    {
		if ( !strcmp( str, getVariableStr(idy) ) )	
		{
		    (*variableobj[idy]) += inp;
		    (*variablenr[idy]) += idx;
		    found = true;
		    break;
		}
	    }

	    if ( !found )
	    {
		variableobj += new TypeSet<int>( 1, inp );
		variablenr += new TypeSet<int>( 1, idx );
	    }
	}
		
	return true;
    }

    return true;
}


static void parens( const char* str, int& idx, int& parenslevel, int len )
{
    while ( idx<len && (str[idx]=='(' || parenslevel) )
    {
	if ( str[idx]=='(' ) parenslevel++;
	if ( parenslevel && str[idx]==')' ) parenslevel--;
	if ( parenslevel ) idx++;
    }
}


MathExpression* MathExpression::parse( const char* input )
{
    int len = strlen( input );
    if ( !len ) return 0;

    ArrPtrMan<char> str = new char[len+1];
    str[0]=0;

    int pos = 0;
    for ( int idx=0; idx<len; idx++ )
    {
	if ( !isspace(input[idx]) )
	    str[pos++] = input[idx];
	str[pos] = 0;
    }

    len = strlen(str);

    int parenslevel = 0;
    for ( int idx=0; idx<len; idx++ )
    {
	if ( str[idx]=='(' ) parenslevel++;
	if ( str[idx]==')' ) parenslevel--;
    }
    if ( parenslevel ) return 0;

    while ( str[0] == '(' && str[len-1]==')' )
    {
	int localparenslevel = 0;
	bool removeparens = true;

	for ( int idx=1; idx<len-1; idx++ )
	{
	    if ( str[idx]=='(' ) localparenslevel++;
	    if ( str[idx]==')' ) localparenslevel--;

	    if ( localparenslevel<0 )
	    {
		removeparens=false;
		break;
	    }
	}

	if ( !removeparens )
	    break;

	ArrPtrMan<char> tmp = new char[len+1];
	strcpy( tmp, &str[1] );
	tmp[len-2] = 0;
	strcpy( str, tmp );

	len = strlen( str );
    }


    // Look for absolute values
    if ( len>3 &&
	 str[0] == '|' && str[1] != '|' &&
	 str[len-1] == '|' && str[len-2] != '|' )
    {
	ArrPtrMan<char> tmp = new char[len+1];
	strcpy( tmp, &str[1] );
	tmp[len-2] = 0;

	MathExpression* inp = parse( tmp );
	if ( inp )
	{
	    MathExpression* res = new MathExpressionAbs;
	    res->setInput( 0, inp );
	    return res;
	}
    }

    parenslevel = 0;
    bool inabs = false;

    // ? :
    for ( int idx=0; idx<len; idx++ )
    {
	absolute( str, idx, inabs);
	if ( inabs ) continue;

	parens(str, idx, parenslevel, len);
	if ( parenslevel ) continue;

	if ( str[idx] == '?' )
	{
	    if ( !idx ) continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    for ( int idy=idx; idy<len; idy++ )
	    {
		absolute( str, idx, inabs)
		if ( inabs ) continue;

		parens(str, idx, parenslevel, len);
		if ( parenslevel ) continue;

		if ( str[idy] == ':' )
		{
		    MathExpression* inp0 = parse( arg0 );
		    if ( !inp0 ) return 0;

		    ArrPtrMan<char> arg1 = new char[len+1];
		    strcpy( arg1, &str[idx+1] );
		    arg1[idy-idx-1] = 0;

		    MathExpression* inp1 = parse( arg1 );
		    if ( !inp1 )
		    {
			delete inp0;
			return 0;
		    }

		    ArrPtrMan<char> arg2 = new char[len+1];
		    strcpy( arg2, &str[idy+1] );

		    MathExpression* inp2 = parse( arg2 );
		    if ( !inp2 )
		    {
			delete inp0;
			delete inp1;
			return 0;
		    }
		
	
		    MathExpression* res = new MathExpressionCondition;

		    res->setInput( 0, inp0 );
		    res->setInput( 1, inp1 );
		    res->setInput( 2, inp2 );

		    return res;
		}
	    }
	}
    }


    // && ||
    for ( int idx=0; idx<len; idx++ )
    {
	absolute( str, idx, inabs)
	if ( inabs ) continue;

	parens(str, idx, parenslevel, len);
	if ( parenslevel ) continue;

	if ( (str[idx]=='&'&&str[idx+1]=='&')||(str[idx]=='|'&&str[idx+1]=='|'))
	{
	    if ( !idx ) continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    ArrPtrMan<char> arg1 = new char[len+1];
	    strcpy( arg1, &str[idx+2] );

	    MathExpression* inp1 = parse( arg1 );

	    if ( !inp1 )
	    {
		delete inp0;
		return 0;
	    }

	    MathExpression* res = 0;	

	    if ( str[idx] == '&' )
		res = new MathExpressionAND;
	    else 
		res = new MathExpressionOR;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }


    // <, >, <=, >=, ==, !=
    for ( int idx=0; idx<len; idx++ )
    {
	absolute( str, idx, inabs)
	if ( inabs ) continue;

	parens(str, idx, parenslevel, len);
	if ( parenslevel ) continue;

	if ( str[idx]=='<' ||  str[idx]=='>' || str[idx]=='=' || str[idx]=='!')
	{
	    if ( !idx ) continue;
	    if ( (str[idx]=='=' || str[idx]=='!') && str[idx+1] != '=' )
		continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    bool twochars = str[idx+1] == '=' ? true : false;
	    ArrPtrMan<char> arg1 = new char[len+1];
	    strcpy( arg1, &str[idx+1+twochars] );

	    MathExpression* inp1 = parse( arg1 );

	    if ( !inp1 )
	    {
		delete inp0;
		return 0;
	    }

	    MathExpression* res = 0;	

	    if ( str[idx] == '<' )
	    {
		if ( str[idx+1] == '=' )
		    res = new MathExpressionLessOrEqual;
		else
		    res = new MathExpressionLess;
	    }
	    else if ( str[idx] == '>' )
	    {
		if ( str[idx+1] == '=' )
		    res = new MathExpressionMoreOrEqual;
		else
		    res = new MathExpressionMore;
	    }
	    else if ( str[idx] == '=' )
		res = new MathExpressionEqual;
	    else
		res = new MathExpressionNotEqual;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }


    // + -
    for ( int idx=0; idx<len; idx++ )
    {
	absolute( str, idx, inabs)
	if ( inabs ) continue;

        parens(str, idx, parenslevel, len);
	if ( parenslevel ) continue;

	if ( str[idx]=='+' ||  str[idx]=='-' )
	{
	    if ( !idx ) continue;
	    if ( str[idx-1]=='(' ) continue;
	    if ( str[idx-1]=='+' ) continue;
	    if ( str[idx-1]=='-' ) continue;
	    if ( str[idx-1]=='*' ) continue;
	    if ( str[idx-1]=='/' ) continue;
	    if ( str[idx-1]=='^' ) continue;
	    if ( idx > 1 && caseInsensitiveEqual( &str[idx-1], "e", 1 )
		&& !isalpha(str[idx-2]) ) continue;

	    ArrPtrMan<char> arg0 = new char[len+1];
	    strcpy( arg0, str );
	    arg0[idx] = 0;

	    MathExpression* inp0 = parse( arg0 );

	    if ( !inp0 ) return 0;

	    ArrPtrMan<char> arg1 = new char[len+1];
	    // Always use the plus operator. 
	    // So '20-4-5' becomes '20+-4+-5'
	    if ( str[idx] == '+' )
		strcpy( arg1, &str[idx+1] );
	    else
		strcpy( arg1, &str[idx] );

	    MathExpression* inp1 = parse( arg1 );

	    if ( !inp1 )
	    {
		delete inp0;
		return 0;
	    }

	    MathExpression* res = new MathExpressionPlus;

	    res->setInput( 0, inp0 );
	    res->setInput( 1, inp1 );

	    return res;
	}
    }

#define mParseOperator( op, clss ) \
    for ( int idx=0; idx<len; idx++ ) \
    { \
	absolute( str, idx, inabs) \
	if ( inabs ) continue; \
 \
	parens(str, idx, parenslevel, len); \
	if ( parenslevel ) continue; \
 \
	if ( str[idx] == op ) \
	{ \
	    if ( !idx ) continue; \
 \
	    ArrPtrMan<char> arg0 = new char[len+1]; \
	    strcpy( arg0, str ); \
	    arg0[idx] = 0; \
	    MathExpression* inp0 = parse( arg0 ); \
	    if ( !inp0 ) return 0; \
 \
	    ArrPtrMan<char> arg1 = new char[len+1]; \
	    strcpy( arg1, &str[idx+1] ); \
	    MathExpression* inp1 = parse( arg1 ); \
	    if ( !inp1 ) \
	    { \
		delete inp0; \
		return 0; \
	    } \
	 \
	    MathExpression* res = new MathExpression##clss; \
 \
	    res->setInput( 0, inp0 ); \
	    res->setInput( 1, inp1 ); \
 \
	    return res; \
	} \
    }

    mParseOperator( '*', Multiply );
    mParseOperator( '/', Divide );
    mParseOperator( '^', Power );


    char* endptr;
    double tres = strtod( str, &endptr );

    if ( endptr != str )
	return new MathExpressionConstant( tres );


#define mParseFunction( func, clss ) { \
    BufferString funcstr(func); funcstr += "("; \
    int funclen = funcstr.size(); \
    if ( !strncasecmp( str, funcstr.buf(), funclen ) && str[len-1] == ')' ) \
    { \
	ArrPtrMan<char> arg0 = new char[len-funclen+1]; \
	strcpy( arg0, str+funclen ); \
	arg0[len-funclen-1] = 0; \
	MathExpression* inp = parse( arg0 ); \
	if ( !inp ) return 0; \
\
	MathExpression* res = new MathExpression##clss; \
	res->setInput( 0, inp ); \
	return res; \
    } \
    }

    // sqrt
    mParseFunction( "sqrt", Sqrt )
    // exp(x) -> e^x
    mParseFunction( "exp", Exp )
    
    // ln (Natural log)  &  log (10log)
    mParseFunction( "ln", NatLog )
    mParseFunction( "log", Log )

//  sin(), asin(), cos(), acos(), tan(), atan()
    mParseFunction( "sin", Sine )
    mParseFunction( "asin", ArcSine )
    mParseFunction( "cos", Cosine )
    mParseFunction( "acos", ArcCosine )
    mParseFunction( "tan", Tangent )
    mParseFunction( "atan", ArcTangent )

    // random number
    mParseFunction( "rand", Random )
    mParseFunction( "randg", GaussRandom )


    if ( (!strncasecmp( str, "min(", 4 ) || 
	  !strncasecmp( str, "max(", 4 ) ||
	  !strncasecmp( str, "sum(", 4 ) ||
	  !strncasecmp( str, "med(", 4 ) ||
	  !strncasecmp( str, "var(", 4 ) ||
	  !strncasecmp( str, "avg(", 4 ) ) && str[len-1] == ')' )
    {
	TypeSet<int> argumentstop;
	for ( int idx=4; idx<len; idx++ )
	{
	    absolute( str, idx, inabs)
	    if ( inabs ) continue;

	    parens(str, idx, parenslevel, len);
	    if ( parenslevel ) continue;

	    if ( str[idx] == ',' || str[idx] == ')' )
	    {
		if ( !idx ) return 0;

		argumentstop += idx;
		if ( str[idx] == ')' ) break;
	    }
	}

	ObjectSet<MathExpression> inputs;

	int prevstop = 3;
	for ( int idx=0; idx<argumentstop.size(); idx++ )
	{
	    ArrPtrMan<char> arg = new char[len+1];
	    strncpy( arg, &str[prevstop+1], argumentstop[idx]-prevstop-1);
	    arg[argumentstop[idx]-prevstop-1] = 0;
	    prevstop = argumentstop[idx];
	    
	    MathExpression* inp = parse( arg );
	    if ( !inp )
	    {
		deepErase( inputs );
		return 0;
	    }

	    inputs += inp;
	}

	MathExpression* res = 0;
	if ( !strncasecmp( str, "max(", 4 ) )
	    res = (MathExpression*) new MathExpressionMax( inputs.size() );
	else if ( !strncasecmp( str, "min(", 4 ) )
	    res = (MathExpression*) new MathExpressionMin( inputs.size() );
	else if ( !strncasecmp( str, "sum(", 4 ) )
	    res = (MathExpression*) new MathExpressionSum( inputs.size() );
	else if ( !strncasecmp( str, "med(", 4 ) )
	    res = (MathExpression*) new MathExpressionMedian( inputs.size() );
	else if ( !strncasecmp( str, "avg(", 4 ) )
	    res = (MathExpression*) new MathExpressionAverage( inputs.size() );
	else if ( !strncasecmp( str, "var(", 4 ) )
	    res = (MathExpression*) new MathExpressionVariance( inputs.size() );

	if ( !res )
	    return res;

	for ( int idx=0; idx<inputs.size(); idx++ )
	    res->setInput( idx, inputs[idx] );

	return res;
    }
	
    // negative variables
    if ( str[0]== '-' )
    {
	MathExpression* inp0 = parse( "-1" );
	if ( !inp0 ) return 0;

	ArrPtrMan<char> arg1 = new char[len+1];
	strcpy( arg1, str+1 );
	MathExpression* inp1 = parse( arg1 );
	if ( !inp1 ) return 0;

	MathExpression* res = new MathExpressionMultiply;
	res->setInput( 0, inp0 );
	res->setInput( 1, inp1 );

	return res;
    }


    if ( !strcasecmp( input, "pi" ) )
	return new MathExpressionConstant( M_PI );

    if ( !strcasecmp( input, "undef" ) )
	return new MathExpressionConstant( mUdf(float) );

    bool isvariable = true;

    for ( int idx=0; idx<len; idx++ )
    {
	if ( (!idx&&isdigit(str[idx])) || !isalnum(str[idx]) )
	{
	    isvariable = str[idx] == '_' ? true : false;
	    break;
	}
    }

    if ( isvariable )
	return new MathExpressionVariable( str );

    return 0;	
}


MathExpression::MathExpression( int sz )
{
    inputs.allowNull();
    for ( int idx=0; idx<sz; idx++ )
	inputs += 0;
}


MathExpression::~MathExpression( )
{
    deepErase( inputs );
    deepErase( variableobj );
    deepErase( variablenr );
}


int MathExpression::getNrVariables() const
{ return variableobj.size(); }


void MathExpression::copyInput( MathExpression* target ) const
{
    int sz = getNrInputs();

    for ( int idx=0; idx<sz; idx++ )
	target->setInput(idx, inputs[idx]->clone() );
}
