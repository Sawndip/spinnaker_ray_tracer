// Points, directions, colours
struct Vector3
{      
    int x, y, z;
};

struct Ray
{
    struct Vector3 origin;
    struct Vector3 direction; // unit vector
};

struct Sphere
{
    struct Vector3  position;
    int             radius;
    struct Vector3  colour;     // Diffuse colour
    int             reflectivity;
    struct Vector3  transparency;
    int             refractiveIndex;
    int             shininess; // Specular highlights
};

struct Vector3 vectorInvert(struct Vector3 a);
struct Vector3 vectorAdd(struct Vector3 a, struct Vector3 b);
struct Vector3 vectorSubtract(struct Vector3 a, struct Vector3 b);
struct Vector3 vectorScalarFraction(struct Vector3 a, int b);
struct Vector3 vectorScalarMultiply(struct Vector3 a, int b);
struct Vector3 vectorFraction(struct Vector3 a, struct Vector3 b);
struct Vector3 vectorMultiply(struct Vector3 a, struct Vector3 b);
int vectorMagnitude(struct Vector3 in);
struct Vector3 vectorNormalise(struct Vector3 in);
int vectorDotProduct(struct Vector3 a, struct Vector3 b);
int vectorUnitDotProduct(struct Vector3 a, struct Vector3 b);
int vectorUnitDotProduct2(struct Vector3 a, struct Vector3 b);
struct Vector3 vectorCrossProduct(struct Vector3 a, struct Vector3 b);
struct Vector3 vectorRotate(struct Vector3 rotated,
                                struct Vector3 rotateAbout,int amount);
struct Vector3 vectorClamp(struct Vector3 in, int loClamp, int hiClamp);
struct Vector3 vectorRefract(struct Vector3 n1, 
                    struct Vector3 s, int mu1, int mu2);
int vectorSum(struct Vector3 inVector);

struct Ray rayNudge(struct Ray ray);

int sphereIntersection(struct Sphere sphere, struct Ray ray);
