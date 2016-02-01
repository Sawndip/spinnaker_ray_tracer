#include "spin1_api.h"
#include "spinn_io.h"
#include "arithmetic.h"

int lower16 = (1<<16)-1;
int upper16 = ~((int)((1<<16)-1));

// Multiplies two numbers between -1.0 and 1.0
int fp_unit_mult(int a, int b)
{
    int sign = (a<0?-1:1)*(b<0?-1:1);
    unsigned int a1 = (unsigned int)(a<0?-a:a);
    unsigned int b1 = (unsigned int)(b<0?-b:b);

    if((a1>>16)>0)
        return sign*b1;
    else if((b1>>16)>0)
        return sign*a1;

    unsigned int  lowera = (a1 & lower16);
    unsigned int  lowerb = (b1 & lower16);

    return sign * (int)(((lowera*lowerb) & upper16)>>16);
}   

// Multiplies two numbers where b is between -1.0 and 1.0
int fp_unit_mult2(int a, int b)
{
    int sign = (a<0?-1:1)*(b<0?-1:1);
    unsigned int a1 = (unsigned int) (a<0?-a:a);
    unsigned int b1 = (unsigned int) (b<0?-b:b);

    if ((b1>>16)>0)
        return sign*a1;

    int lowera = (a1 & lower16);
    int uppera = ((a1 & upper16)>>16);

    return sign * (((lowera*b1)>>16) + uppera*b1);
}

// Multiplies two numbers where b is between 0.0 and 1.0
int fp_frac(int a, int b)
{
    if ((b>>16)>0)
        return a;

    int sign = (a<0?-1:1);
    unsigned int a1 = (unsigned int) (a<0?-a:a);
    unsigned int b1 = (unsigned int) b;

    int lowera = (a1 & lower16);
    int uppera = ((a1 & upper16)>>16);

    return sign * (((lowera*b1)>>16) + uppera*b1);
}

// Multiplies two numbers. Probably inefficient.
int fp_mult(int a, int b)
{
    int sign = (a<0?-1:1)*(b<0?-1:1);
    unsigned int a1 = (unsigned int)(a<0?-a:a);
    unsigned int b1 = (unsigned int)(b<0?-b:b);

    unsigned int  lowera = (a1 & lower16);
    unsigned int  lowerb = (b1 & lower16);
    unsigned int  uppera = ((a1 & upper16)>>16);
    unsigned int  upperb = ((b1 & upper16)>>16);

    int mult1 = (int)(((lowera*lowerb) & upper16)>>16);
    int mult2 = (int)(lowera*upperb + lowerb*uppera);
    int mult3 = (int)(((uppera*upperb) & lower16)<<16);

    return (mult1+mult2+mult3)*sign;
}

// Estimate the reciprocal using the Newton-Raphson method of finding zeroes
// of real-valued functions. Number of correct bits in estimate squares per
// iteration. To ensure a good estimate after 2 iterations, the argument given
// should be between 0.5 and 1.0
int fp_newtons_reciprocal(int a)
{
    // This is the optimal starting estimate for the reciprocal of values known
    // to be in the range 0.5 to 1.0
    int estimate    = 185043 - fp_frac(123362, a);

    // Two iterations of the method should suffice.
    estimate        = fp_mult(estimate, 131072 - fp_frac(estimate, a) );
    estimate        = fp_mult(estimate, 131072 - fp_frac(estimate, a) );

    return estimate;
}

// Find the reciprocal of a number by shifting it until it lies between
// 0.5 and 1.0, then call the method above to use Newton's method
int fp_reciprocal(int a)
{
    if (a==0)
    {
        io_printf(IO_STD, "Fixed-point arithmetic error: div by 0.\n");
    }

    int sign = (a<0?-1:1);
    unsigned int a1 = (unsigned int)(a<0?-a:a);

    int shifted = 0;

    while (a1>65536)
    {
        a1>>=1;
        shifted -= 1;
    }
    while (a1<=32768)
    {
        a1<<=1;
        shifted +=1 ;
    }
    a1 = fp_newtons_reciprocal(a1);
    a1 = (shifted>0 ? (a1<<shifted) : (a1>>-(shifted)) );
    return sign*((int)a1);
}

// To divide the numerator by the denominator, estimate the reciprocal of the
// denominator and multiply the two. The function for finding the reciprocal
// only works on denominators between 0.5 and 1, so we have to bitshift both
// arguments until it lies in this range.
int fp_div(int num, int denom)
{
    if (denom==0)
    {
        io_printf(IO_STD, "Fixed-point arithmetic error: div by 0.\n");
    }

    // fp_newtons_reciprocal works only on positive values
    if (denom<0)
    {
        num=-num;
        denom=-denom;
    }

    // fp_newtons_reciprocal works only on values in range 0.5 to 1.0
    while(denom>65536) // while greater than 1
    {
        num>>=1;
        denom>>=1;
    }
    while(denom<=32768) // while less than half
    {
        num<<=1;
        denom<<=1;
    }

    return fp_mult(num,fp_newtons_reciprocal(denom));
}

// Ken Turkowski's implementation of fast fixed point square root.
// Don't ask me how it works, but it's faster than any function I wrote for it
// (Note: square roots are still expensive Anyone looking to optimize
// should start here)
int fp_sqrt(int x)
{
    unsigned int root, remHi, remLo, testDiv, count;

    root = 0;
    remHi = 0;
    remLo = x;
    count = (15 + (16>>1));

    do
    {
        // Get 2 bits of arg
        remHi = (remHi << 2) | (remLo >> 30); remLo <<= 2;
        root <<= 1;                 // Get ready for the next bit in the root
        testDiv = (root << 1) + 1;  // Test radical
        if (remHi >= testDiv) {
            remHi -= testDiv;
            root += 1;
        }
    }   while (count-- != 0);
    return(root);
}


// A look-up table  for cos(x), with x in range 0.0 to Pi/2 (1/4 of the cycle)
// (Note: the mapping might be slighly off, knocking some angles off by about
// a degree.  Warrants investigation.)
int cos_lookup[101] = {65536, 65527, 65503, 65463, 65406, 65333, 65245, 65140,
65019, 64882, 64729, 64560, 64375, 64174, 63957, 63725, 63477, 63213, 62933,
62638, 62328, 62002, 61661, 61305, 60933, 60547, 60145, 59729, 59298, 58853,
58393, 57918, 57429, 56926, 56409, 55878, 55333, 54775, 54203, 53618, 53019,
52408, 51783, 51146, 50496, 49833, 49159, 48472, 47773, 47063, 46340, 45607,
44862, 44106, 43339, 42562, 41774, 40975, 40167, 39349, 38521, 37683, 36836,
35980, 35115, 34242, 33360, 32470, 31572, 30666, 29752, 28831, 27903, 26969,
26027, 25079, 24125, 23165, 22199, 21228, 20251, 19270, 18283, 17293, 16298,
15299, 14296, 13289, 12280, 11267, 10252, 9234, 8213, 7191, 6167, 5141, 4115,
3087, 2058, 1029, 0};

#define ONE_OVER_2_PI   10430

// Transforms a into an index for the above lookup table.  The table covers
// 1/4 of the cycle of the cos function.  For the other quarters, either
// the looked-up value, the look-up index, or both are flipped.
int fp_cos(int a)
{
    int lookup_size = 100;
    int factor = (65535/(lookup_size*4-1));

    // a is an angle in rads
    // We only care about it in the range 0 -> 2*pi

    // this is between 0 and 65536
    int unscaledIndex = (fp_frac(a, ONE_OVER_2_PI) & lower16);

    int indexFraction = unscaledIndex % factor;
    // This is now a lookup index between 0 and lookup_size*4-1
    int index = unscaledIndex / factor;
    // Second index for interpolation
    int index2 = (index+1)%(lookup_size*4);
    // How far between the 2 indices does true angle lie? (for interpolation)
    indexFraction = indexFraction & lower16;

    // Lookup value 1 for interpolation
    // Flip the value and/or index as required
    int sign = 1;
    if (index >= lookup_size && index < lookup_size*2)
    {
        index = lookup_size - 1 - (index % lookup_size);
        sign = -1;
    }
    else if (index >= lookup_size*2 && index < lookup_size*3)
    {
        index = index % lookup_size;
        sign = -1;
    }
    else if (index >= lookup_size*3 && index < lookup_size*4)
    {
        index = lookup_size - 1 - (index % lookup_size);
    }
    int value = sign * cos_lookup[index];

    // Lookup value 2 for interpolation
    // Flip the value and/or index as required
    int sign2 = 1;
    if (index2 >= lookup_size && index2 < lookup_size*2)
    {
        index2 = lookup_size - (index2 % lookup_size);
        sign2 = -1;
    }
    else if (index2 >= lookup_size*2 && index2 < lookup_size*3)
    {
        index2 = index2 % lookup_size;
        sign2 = -1;
    }
    else if (index2 >= lookup_size*3 && index2 < lookup_size*4)
    {
        index2 = lookup_size - (index2 % lookup_size);
    }
    int value2 = sign2 * cos_lookup[index2];

    // Interpolate between the two values
    return (value2*indexFraction)/factor + \
    (value*(factor-indexFraction))/factor;
}
