#ifndef RANDOM_H
#define RANDOM_H

#include <math.h>
#include <vector>


//
// Diese Klasse stellt Methoden zum Generieren von Pseudo-Zufallszahlen bereit.
//
class c_Random {
    int inext,inextp;
    long ma[56];
public:
    c_Random( const int initial_value = -1 );

    // a uniform [0,1) random number
    double uniform_random();
    // a double random number in [lower, upper)
    double dran( const double lower, const double upper )
        { return ( lower + ( upper - lower ) * uniform_random () ); };
    // a integer random number in the integer interval [lower, upper]
    int iran( const int lower, const int upper )
        { return ( (int) floor ( (double) lower + ( 1.0 + (double) upper - (double) lower ) * uniform_random () ) ); };
    // generates p different integers from [1, f] and stores them in result[0, f-1]
    void generate_p_from_f_different_randoms( const int p, const int f, int* &result ); // Array result needs to be deleted afterwards!
    // constructs a permutation of 0, 1, ..., perm.size()-1
    void generate_permutation( std::vector<int>& perm );
};

#endif // RANDOM_H
