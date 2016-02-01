#define FP_PI       205824

// All integers represent fixed point numbers with 16 bits for the integer
// part and 16 bits for the fractional

// Note: There are multiple multiplication functions to take advantage of
// the user's knowledge about the numbers being multiplied. E.g. If an operand
// always has an empty upper 16 bits, we can save some operations.

int fp_unit_mult(int a, int b);
int fp_unit_mult2(int a, int b);
int fp_frac(int a, int b);
int fp_mult(int a, int b);
int fp_newtons_reciprocal(int a);
int fp_reciprocal(int a);
int fp_div(int num, int denom);
int fp_sqrt(int x);
int fp_cos(int a);
