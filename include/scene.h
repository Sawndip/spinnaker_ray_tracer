
// Camera, with viewplane
struct ViewPlane
{   // these are unit vectors and represent the direction to the four corners of the view plane from the camera
    struct Vector3 topLeft;
    struct Vector3 topRight;
    struct Vector3 bottomLeft;
    struct Vector3 bottomRight;
};

struct Camera
{
    struct Vector3      position;
    struct Vector3      lookDirection;
    struct Vector3      upDirection;

    int                 horizontalFieldOfView;
    int                 verticalFieldOfView;
    int                 horizontalPixels;
    int                 verticalPixels;

    int                 antialiasing;

    struct Vector3      rightDirection;
    struct ViewPlane    viewPlane;  // The two view angles are used to construct the viewplane
};

// Point lights
struct Light
{
    struct Vector3 position;
    struct Vector3 colour;
};

struct Camera setupCamera(struct Camera camera);
