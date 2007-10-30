#ifndef pca_h
#define pca_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: pca.h,v 1.8 2007-10-30 16:53:35 cvskris Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
#include "sets.h"
#include "trigonometry.h"

template <class T> class Array2D;
namespace Threads { class ThreadWorkManager; };
class SequentialTask;
class PCACovarianceCalculator;

/*!\brief
Performs Pricipal Component Analysis on samples with N variables.

Example of usage:\code
   //The samples will have three variables
   PCA pca( 3 );		

   //Samples can be added by any object that has a readable [] operator
   const float sample0[] = { 0, 1, 2 };
   pca.addSample( sample0 );

   const float sample1[] = { 4.343, 9.8, 2.72 };
   pca.addSample( sample1 );

   const float sample2[] = { 23.15, 210, -15 };
   pca.addSample( sample2 );

   const float sample3[] = { -0.36, 0.68, 3 };
   pca.addSample( sample3 );

   const float sample4[] = { 4.4, 9,6, 11 };
   pca.addSample( sample4 );

   TypeSet<float> sample5; sample5 += 34.1; sample5 += 8.37; sample5 += -44;
   pca.addSample( sample5 ); 

   pca.calculate();


   //Any object that has a writable [] operator can be used to fetch
   //the resulting vectors:
   TypeSet<float> eigenvec0(3,0);
   float eigenval0 = pca.getEigenValue(0);
   pca.getEigenVector( 0, eigenvec0 );

   float[3] eigenvec1;
   float eigenval1 = pca.getEigenValue(1);
   pca.getEigenVector( 1, eigenvec1 );

   float[3] eigenvec2;
   float eigenval2 = pca.getEigenValue(2);
   pca.getEigenVector( 2, eigenvec2 );
\endcode


*/

class PCA
{
public:
    					PCA( int nrvars );
					/*!<\param nrvars The number of
						  variables that the samples
						  have. */
    virtual				~PCA();

    void				clearAllSamples();
    					/*!< Removes all samples so a new
					     analysis can be made (by adding
					     new samples) */
    template <class IDXABL> void	addSample( const IDXABL& sample );
    					/*!<Adds a sample to the analysis.
					    \param sample The sample that should
					    		  be added. The sample
							  can be of any type
							  that have []
							  operators.  */
    bool				calculate();
    					/*!< Computes the pca for all
					     added samples. */
    float				getEigenValue(int idx) const;
    					/*!<\return an eigenvalue.
					  \param idx Determines which 
					  	eigenvalue to return. The
						eigenvalues are sorted in
						descending order, so idx==0
						gives the largest eigenvalue. */
    template <class IDXABL> void	getEigenVector(int idx,
	    					       IDXABL& vec) const;
    					/*!<Returns the eigenvector
					    corresponding to the eigenvalue
					    \a idx.
					    \param idx Determines which
					    	eigenvector to return. The
						eigenvectors are sorted in
						order of descending eigenvalue,
						so idx==0 gives the eigenvector
						corresponding to the largest
						eigenvalue.
					    \param vec The object where the to
					     	       store the eigenvector.
						       Any object that has a
						       writable [] operator can
						       be used, for example
						       float* and TypeSet<T>.
					*/

    void				setThreadWorker(
	    					Threads::ThreadWorkManager* );
    					/*!< Enables multi-threaded calculation,
					     which is beneficial when the
					     number of samples and/or number
					     of variables are large. The object
					     does however work without setting
					     the threadworkmanager.
					     \note threadworkmanager is managed
					     (i.e. deleted) by caller, and it
					     can be shared with many other
					     objects (such as other PCAs).
					*/

protected:
    bool			tqli( float[], float[], int, ObjectSet<float>&);
    void			tred2( ObjectSet<float>&, int, float[],float[]);

    const int			nrvars;
    Array2DImpl<float>		covariancematrix;
    ObjectSet<TypeSet<float> >	samples;
    TypeSet<float>		samplesums;
    Threads::ThreadWorkManager*	threadworker;
    ObjectSet<SequentialTask>	tasks;
    float*			eigenvalues;
    				/*!<The negation of the eigenval,
    				    to get the sorting right.
				*/
    int*			eigenvecindexes;
};


template <class IDXABL> inline
void PCA::addSample( const IDXABL& sample )
{
    TypeSet<float>& ownsample = *new TypeSet<float>;
    for ( int idx=0; idx<nrvars; idx++ )
    {
	const float val = sample[idx];
	ownsample += val;
	samplesums[idx] += val;
    }

    samples += &ownsample;
}

template <class IDXABL> inline
void PCA::getEigenVector(int idy, IDXABL& vec ) const
{
    for ( int idx=0; idx<nrvars; idx++ )
	vec[idx] = covariancematrix.get(idx, eigenvecindexes[idy] );
}

#endif

