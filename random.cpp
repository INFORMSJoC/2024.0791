/**** Include eigenes Modul ****/

#include "random.h"


/**** Implementierung ****/

const int MBIG  = 1000000000;       // "groß M", "BIG_M"
const int MSEED = 161803398;        // Seed der Zufallszahlenberechnung
const int MZ = 0;

/**** FUNCTION  FAC  ***********************************************************

---- DESCRIPTION   -------------------------------------------------------------

---- PARAMETERS  ---------------------------------------------------------------
1.0/MBIG

---- RETURNS  ------------------------------------------------------------------

*******************************************************************************/

const double FAC = (1.0/MBIG);      // Umkehrwert von "BIG_M"

/* According to Knuth, any large MBIG, and any smaller (but still large) */
/* MSEED can be substituted for the above values.                         */

/**** FUNCTION  ran3 ***********************************************************

---- DESCRIPTION   -------------------------------------------------------------
Returns a uniform random deviate between 0.0 and 1.0.

---- RETURNS  ------------------------------------------------------------------
uniform number in (0.0; 1.0).

*******************************************************************************/

c_Random::c_Random ( const int initial_value )
{
    if ( initial_value >= 0 )
        throw;

    long mj,mk;
    int i,ii,k;

    int idum = initial_value;
    // Initialization
    mj = MSEED-( idum < 0 ? -idum : idum);
    mj %= MBIG;
    ma [55] = mj;
    mk=1;
    for (i=1;i<=54;i++)
    {
        // Now Initialize the rest of the table
        // in a slightly random order
        // with numbers that are not especially random

        ii=(21*i) % 55;
        ma[ii] = mk;
        mk  = mj-mk;
        if (mk < MZ) mk += MBIG;
        mj = ma[ii];
    }

    // We randomize them by "warming up the generator."
    for (k=1;k<=4;k++)
        for (i=1;i<=55;i++)
        {
            ma[i] -= ma[1+(i+30) % 55];
            if (ma[i] < MZ)
                ma[i] += MBIG;
        }
    // Prepare indices for our first generated number.
    // The constant 31 is special; see Knuth.
    inext=0;
    inextp=31;
}



double c_Random::uniform_random( )
{
    if (++inext == 56) inext=1;
    // Here is where we start, except on initialization. Increment inext,
    // wrapping around 56 to 1
    if (++inextp == 56) inextp=1;   // Ditto for inextp.
    long mj = ma[inext]-ma[inextp]; // Now generate a new random
                                    // number subtractively
    if (mj < MZ) mj += MBIG;        // Be sure that it is in range.
    ma[inext] = mj;                 // Store it,
    return mj*FAC;                  // and output the derived uniform deviate.
}

void c_Random::generate_p_from_f_different_randoms( const int p, const int f, int* &result )
{
    result = new int[ p ];

    bool* labeled = new bool[ f ];

    for( int i = 0; i < f; i++ )
        labeled[ i ] = false;

    for( int i = 0; i < p; i++ )
    {
        const int random_number = iran( 1, f - i );

        int j = 0;
        int count = 0;
        while( count < random_number )
        {
            if( ! labeled[ j ] )
                count++;
            j++;
        }
        j--;

        labeled[ j ] = true;
        result[ i ] = j + 1;
    }

    delete[] labeled;
};





void c_Random::generate_permutation( std::vector<int>& perm )
{
    int size = (int) perm.size();
    for ( int i=0; i<size; i++ )
        perm[i] = -1;
    int curr_pos = 0;
    for ( int i=0; i<size; i++ )
    {
        curr_pos = ( curr_pos + iran( 0, size-1 ) ) % size;
        while ( perm[curr_pos] >= 0 )
            curr_pos = ( curr_pos + 1 ) % size;
        perm[ curr_pos ] = i;
    }
}
