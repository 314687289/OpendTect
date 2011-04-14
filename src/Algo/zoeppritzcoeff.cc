/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 1999
-*/


#include "zoeppritzcoeff.h"
#include "ailayer.h"


ZoeppritzCoeff::ZoeppritzCoeff()
    : pdn_pdn_( 0, 0 )
    , pdn_sdn_( 0, 0 )
    , pdn_pup_( 0, 0 )
    , pdn_sup_( 0, 0 )
    , sdn_pdn_( 0, 0 )
    , sdn_sdn_( 0, 0 )
    , sdn_pup_( 0, 0 )
    , sdn_sup_( 0, 0 )
    , pup_pdn_( 0, 0 )
    , pup_sdn_( 0, 0 )
    , pup_pup_( 0, 0 )
    , pup_sup_( 0, 0 )
    , sup_pdn_( 0, 0 )
    , sup_sdn_( 0, 0 )
    , sup_pup_( 0, 0 )
    , sup_sup_( 0, 0 )
{}


void ZoeppritzCoeff::setInterface( float p, 
				const AILayer& pl1, const AILayer& pl2, 
				const AILayer& sl1, const AILayer& sl2 )	
{
    float p2 = p * p; 
    float pvel1 = pl1.vel_;
    float pvel2 = pl2.vel_;
    float svel1 = sl1.vel_;
    float svel2 = sl2.vel_;
    
    bool waterabove = mIsZero(svel1,mDefEps);	//Detect water
    bool waterbelow = mIsZero(svel2,mDefEps);

    if ( waterabove ) svel1 = 0.1;	// Set small values to make eqns work
    if ( waterbelow ) svel2 = 0.1;

    float l1s2 = svel1 * svel1;
    float l2s2 = svel2 * svel2;
    float l1p2 = pvel1 * pvel1;
    float l2p2 = pvel2 * pvel2;

    float_complex a = pl2.den_ * (1 -  2 * l2s2 * p2) -
	      pl1.den_ * (1 -  2 * l1s2 * p2);	
    float_complex b = pl2.den_ * (1 -  2 * l2s2 * p2) +
	      pl1.den_ * 2 * l1s2 * p2;	
    float_complex c = pl1.den_ * (1 -  2 * l1s2 * p2) +
	      pl2.den_ * 2 * l2s2 * p2;	
    float_complex d = 2 * (pl2.den_ * l2s2 - pl1.den_ * l1s2);

    float_complex pzi1 = sqrt( float_complex( 1/l1p2 - p2, 0) );
    float_complex pzi2 = sqrt( float_complex( 1/l2p2 - p2, 0) );

    float_complex pzj1 = sqrt( float_complex( 1/l1s2 - p2, 0) );
    float_complex pzj2 = sqrt( float_complex( 1/l2s2 - p2, 0) );

    float_complex ee = b * pzi1 + c * pzi2;
    float_complex ff = b * pzj1 + c * pzj2;
    float_complex gg = a - d * pzi1 * pzj2;
    float_complex hh = a - d * pzi2 * pzj1;
    float_complex dd = ee*ff + gg*hh * p2;

    const float f2 = (float)2;

    pdn_pup_ = ( (b*pzi1 - c*pzi2) * ff - 
				(a + d * pzi1 * pzj2) * hh * p2) / dd;
    pdn_pdn_ = 2 * pl1.den_ * pzi1 * ff * pvel1/(pvel2*dd);

    pdn_sup_ = -f2 * pzi1 * ( a*b + c*d * pzi2*pzj2 ) 
				* p * pvel1 /(svel2 *dd);
    pdn_sdn_ = 2 * pl1.den_ * pzi1 * hh * p * pvel1/(svel2*dd);

    sdn_pup_ = -f2 * pzj1 * (a*b + c*d * pzi2*pzj2) * p * svel1/(pvel1*dd);
    sdn_pdn_ = -2 * pl1.den_ * pzj1 * gg * 
				p * svel1/(pvel2*dd);
    sdn_sup_ = -( (b*pzj1 - c*pzj2) * ee - 
				(a + d*pzi2*pzj1) * gg*p2) /dd;
    sdn_sdn_ = 2 * pl1.den_ * pzj1 * ee * svel1/(svel2*dd);
    pup_pup_ = 2 * pl2.den_ * pzi2 * ff * pvel2/(pvel1*dd);
    pup_pdn_ = -( ( b*pzi1 - c*pzi2 ) * ff + 
				(a + d*pzi2*pzj1) * gg * p2)/dd;
    pup_sup_ = -2 * pl2.den_ * pzi2 * gg * p * pvel2/(svel1*dd);
    pup_sdn_ = f2 * pzi2 * ( a*c + b*d*pzi1*pzj1) 
				* p * pvel2/(svel2*dd);

    sup_pup_ = 2 * pl2.den_ * pzj2 * hh * p * svel2/(pvel1*dd);
    sup_pdn_ = f2 * pzj2 * (a*c + b*d * pzi1 * pzj1)  
				* p * svel2/(pvel2*dd);
    sup_sup_ = 2 * pl2.den_ * pzj2 * ee * svel2/(svel1*dd);
    sup_sdn_ = ( (b*pzj1 - c*pzj2) * ee + 
				(a + d*pzi1*pzj2) * hh*p2)/dd;

    if ( waterabove )
    {
	sdn_pdn_ = 0;
	sdn_sdn_ = 0;
	sdn_pup_ = 0;
	sdn_sup_ = 0;

	pdn_sup_ = 0;
	pup_sup_ = 0;
	sup_sup_ = 0;
    }

    if ( waterbelow )
    {
	sdn_pdn_ = 0;
	sdn_sdn_ = 0;
	sdn_pup_ = 0;
	sdn_sup_ = 0;

	pdn_sdn_ = 0;
	pup_sdn_ = 0;
	sup_sdn_ = 0;
    }
} 


float_complex ZoeppritzCoeff::getCoeff( bool downin, bool downout,
					bool pin, bool pout ) const
{
    if ( downin )
    {
	if ( pin )
	{
	    if ( downout )
	    {
		if ( pout )
		    return pdn_pdn_;

		return pdn_sdn_;
	    }

	    if ( pout )
		return pdn_pup_;

	    return pdn_sup_;
	}
	
	if ( downout )
	{
	    if ( pout )
		return sdn_pdn_;

	    return sdn_sdn_;
	}
	
	if ( pout )
	    return sdn_pup_;

	return sdn_sup_;
    }

    if ( pin )
    {
	if ( downout )
	{
	    if ( pout )
		return pup_pdn_;

	    return pup_sdn_;
	}

	if ( pout )
	    return pup_pup_;

	return pup_sup_;
    }

    if ( downout )
    {
	if ( pout )
	    return sup_pdn_;

	return sup_sdn_;
    }

    if ( pout )
	return sup_pup_;

    return sup_sup_;
}



float_complex getFastCoeff(float p, 
			const AILayer& player1, const AILayer& player2, 
			const AILayer& slayer1, const AILayer& slayer2 )
{
    const float vp1 = player1.vel_; 
    const float vp2 = player2.vel_;
    const float dp1 = player1.den_; 
    const float dp2 = player2.den_;

    const float vs1 = slayer1.vel_; 
    const float vs2 = slayer2.vel_;
    const float ds1 = slayer1.den_; 
    const float ds2 = slayer2.den_;

    const float AIp1 = vp1*dp1;
    const float AIp2 = vp2*dp2;
    const float AIs1 = vs1*ds1;
    const float AIs2 = vs2*ds2;

    const float RAIp = ( AIp2 - AIp1 )/ ( AIp1 + AIp2 );
    const float RAIs = ( AIs2 - AIs1 )/ ( AIs1 + AIs2 );

    const float Vp = ( vp2 + vp1 ) / 2;
    const float Vs = ( vs1 + vs2 ) / 2;
    const float DVp = ( vp2 - vp1 );
    const float DVs = ( vs2 - vs1 );

    const float P = ( dp1 + dp2 ) / 2;
    const float Dp = ( dp1 - dp2 );
    const float RD = Dp / (2*P);

    const float sinangle = p * vp1;
    const float angle = asin( sinangle );
    const float sin2i = sinangle * sinangle;
    const float tan2i = tan( angle )*tan( angle );

    const float A = 0.5*( 1-4*( Vs*Vs/(Vp*Vp) )*sin2i )*Dp/P;
    const float B = 0.5*(1+tan2i)*DVp/Vp;
    const float C = 4*sin2i*Vs*Vs*DVs/( Vp*Vp*Vs );

    return float_complex( A+B+C, 0 );
}
