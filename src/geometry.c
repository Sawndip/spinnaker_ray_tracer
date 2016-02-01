#include "arithmetic.h"
#include "geometry.h"

//-------- Vector functions ---------------------------------------------------

struct Vector3 vectorInvert(struct Vector3 a)
{
    struct Vector3 result = {-a.x, -a.y, -a.z};
    return result;
}

struct Vector3 vectorAdd(struct Vector3 a, struct Vector3 b)
{
    struct Vector3 result = {a.x+b.x, a.y+b.y, a.z+b.z};
    return result;
}

struct Vector3 vectorSubtract(struct Vector3 a, struct Vector3 b)
{
    struct Vector3 result = {a.x-b.x, a.y-b.y, a.z-b.z};
    return result;
}

// Note: For some functions there are multiple copies to take advantage of
// the users knowledge of the variables involved. E.g., due to fixed point
// arithmetic, we can save time if we know that one of the operands always
// has an absolute value less than 1.0 (by skipping two of the integer
// multiplications)

// Multiply a vector by a scalar value between 0.0 and 1.0
struct Vector3 vectorScalarFraction(struct Vector3 a, int b)
{
    struct Vector3 result = {fp_frac(a.x,b), fp_frac(a.y,b), fp_frac(a.z,b)};
    return result;
}

// Multiply a vector by a scalar value
struct Vector3 vectorScalarMultiply(struct Vector3 a, int b)
{
    struct Vector3 result = {fp_mult(a.x,b), fp_mult(a.y,b), fp_mult(a.z,b)};
    return result;
}

// Element-wise multiplication of two vectors where the elements of vector b
// are between 0.0 and 1.0
struct Vector3 vectorFraction(struct Vector3 a, struct Vector3 b)
{
    struct Vector3 result = \
        {fp_frac(a.x,b.x), fp_frac(a.y,b.y), fp_frac(a.z,b.z)};
    return result;
}

// Element-wise multiplication of two vectors
struct Vector3 vectorMultiply(struct Vector3 a, struct Vector3 b)
{
    struct Vector3 result = \
        {fp_mult(a.x,b.x), fp_mult(a.y,b.y), fp_mult(a.z,b.z)};
    return result;
}

int vectorMagnitude(struct Vector3 in)
{
    return fp_sqrt(fp_mult(in.x,in.x)+fp_mult(in.y,in.y)+fp_mult(in.z,in.z));
}

struct Vector3 vectorNormalise(struct Vector3 in)
{
    int vectorMagnitudeReciprocal = fp_reciprocal(vectorMagnitude(in));

    if (vectorMagnitudeReciprocal>(1<<16))
    {
        struct Vector3 result = {   fp_mult(in.x,vectorMagnitudeReciprocal),
                                    fp_mult(in.y,vectorMagnitudeReciprocal),
                                    fp_mult(in.z,vectorMagnitudeReciprocal) };
        return result;
    }
    else
    {
        struct Vector3 result = {   fp_frac(in.x,vectorMagnitudeReciprocal),
                                    fp_frac(in.y,vectorMagnitudeReciprocal),
                                    fp_frac(in.z,vectorMagnitudeReciprocal) };
        return result;
    }
}

// The dot product of two vectors
int vectorDotProduct(struct Vector3 a, struct Vector3 b)
{
    return fp_mult(a.x,b.x) + fp_mult(a.y,b.y) + fp_mult(a.z,b.z);
}

// The dot product of two vectors that we know to be unit vectors
int vectorUnitDotProduct(struct Vector3 a, struct Vector3 b)
{
    return fp_unit_mult(a.x,b.x) + \
            fp_unit_mult(a.y,b.y) + fp_unit_mult(a.z,b.z);
}

// The dot product of two vectors where b is known to be a unit vector
int vectorUnitDotProduct2(struct Vector3 a, struct Vector3 b)
{
    return fp_unit_mult2(a.x,b.x) + \
            fp_unit_mult2(a.y,b.y) + fp_unit_mult2(a.z,b.z);
}

// The cross product of two vectors
struct Vector3 vectorCrossProduct(struct Vector3 a, struct Vector3 b)
{
    struct Vector3 result = {   fp_mult(a.y,b.z)-fp_mult(a.z,b.y),
                                fp_mult(a.z,b.x)-fp_mult(a.x,b.z),
                                fp_mult(a.x,b.y)-fp_mult(a.y,b.x)   };
    return result;
}

// Rotate the first vector around the second (amount is in degrees)
struct Vector3 vectorRotate(struct Vector3 rotated,
                                struct Vector3 rotateAbout,int amount)
{
    amount = fp_mult(amount,FP_PI)/180;
    int c = fp_cos(amount);
    int s = fp_cos(amount - (FP_PI>>1));
    int t = (1<<16)-c;

    // The rotation (sorry about the readability!)
    struct Vector3 result;
    result.x = \
        fp_mult(rotated.x,(fp_mult(t,fp_mult(rotateAbout.x,rotateAbout.x))+c))\
        + fp_mult(rotated.y,(fp_mult(t,fp_mult(rotateAbout.x,rotateAbout.y)) \
            + fp_mult(s,rotateAbout.z))) + \
        fp_mult(rotated.z,(fp_mult(t,fp_mult(rotateAbout.x,rotateAbout.z)) \
            - fp_mult(s,rotateAbout.y)));

    result.y = \
        fp_mult(rotated.x,(fp_mult(t,fp_mult(rotateAbout.x,rotateAbout.y)) \
            - fp_mult(s,rotateAbout.z))) + \
        fp_mult(rotated.y,(fp_mult(t,fp_mult(rotateAbout.y,rotateAbout.y)) \
            + c)) + \
        fp_mult(rotated.z,(fp_mult(t,fp_mult(rotateAbout.y,rotateAbout.z)) \
            + fp_mult(s,rotateAbout.x)));

    result.z = \
        fp_mult(rotated.x,(fp_mult(t,fp_mult(rotateAbout.z,rotateAbout.x)) \
            + fp_mult(s,rotateAbout.y))) + \
        fp_mult(rotated.y,(fp_mult(t,fp_mult(rotateAbout.y,rotateAbout.z)) \
            - fp_mult(s,rotateAbout.x))) + \
        fp_mult(rotated.z,(fp_mult(t,fp_mult(rotateAbout.z,rotateAbout.z))+c));

    return vectorNormalise(result);
}

// Clamp the elements of a vector between a lower and upper value
struct Vector3 vectorClamp(struct Vector3 in, int loClamp, int hiClamp)
{
    struct Vector3 clamped = {
        (in.x > hiClamp) ? hiClamp : ((in.x < loClamp) ? loClamp : in.x), 
        (in.y > hiClamp) ? hiClamp : ((in.y < loClamp) ? loClamp : in.y), 
        (in.z > hiClamp) ? hiClamp : ((in.z < loClamp) ? loClamp : in.z)
    };

    return clamped;
}

// Calculate the refraction when vector n1 goes from a material with refractive
// index mu1 into one with refractive index mu2, via a surface with normal s
// (s goes in the direction in to the new material, so may actually be the
// inverse of the collision-object surface normal depending on if the ray is
// entering or exiting the object)
struct Vector3 vectorRefract(struct Vector3 n1, 
                    struct Vector3 s, int mu1, int mu2)
{
    int n1DotS      = vectorUnitDotProduct(n1,s);
    struct Vector3 firstTerm   = vectorScalarMultiply(s,n1DotS);
    int sqrtTerm    = fp_sqrt(  (fp_mult(mu2,mu2)) - \
                                (fp_mult(mu1,mu1)) + \
                                (fp_mult(n1DotS,n1DotS)) );
    struct Vector3 result = \
        vectorAdd( vectorSubtract(n1, firstTerm),
            vectorScalarMultiply(s, sqrtTerm) );

    return result;  
}

int vectorSum(struct Vector3 inVector)
{
    return inVector.x+inVector.y+inVector.z;
}

//-------- Vectors ------------------------------------------------------------




//-------- Ray functions ------------------------------------------------------

// Nudges a ray along slightly in its direction vector.  Used to get rays away
// from the surfaces on which they have just reflected/refracted to avoid re-
// interacting with the same surface
struct Ray rayNudge(struct Ray ray)
{
    ray.origin = \
        vectorAdd(ray.origin, vectorScalarFraction(ray.direction,1<<10));
    return ray;
}

//-------- Rays ---------------------------------------------------------------





//-------- Sphere functions ---------------------------------------------------

// If the ray intersects the sphere, return nearest distance along ray at which
// it happens. If it doesn't, return -1
int sphereIntersection(struct Sphere sphere, struct Ray ray)
{
    // The intersection algorithm assumes that the ray starts at the origin,
    // so translate the sphere first.
    sphere.position = vectorSubtract(sphere.position, ray.origin);

    int directionDotPosition = \
        vectorUnitDotProduct2(sphere.position, ray.direction);
    int sqrtTerm = fp_mult(directionDotPosition,directionDotPosition) \
                    - vectorDotProduct(sphere.position, sphere.position) \
                    + fp_mult(sphere.radius,sphere.radius);
    
    int result = -1; // Return value for no intersection

    if (sqrtTerm >= 0) // If there is an intersection (more likely 2)
    {
        sqrtTerm = fp_sqrt(sqrtTerm);
        // There are usually two solutions, for the two intersection points
        // between the ray and sphere.
        int solution1 = directionDotPosition + sqrtTerm;
        int solution2 = directionDotPosition - sqrtTerm;

        // We want the nearest non-negative (behind the ray origin) intersection
        if (solution1 >= 0)
        {
            result = solution1;
            if (solution2 >= 0 && solution2 < solution1)
                result = solution2;
        }
        else if (solution2 >= 0)
            result = solution2;
    }

    return result;
}

//-------- Spheres ------------------------------------------------------------
