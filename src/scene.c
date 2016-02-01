#include "geometry.h"
#include "scene.h"

// Given a camera with a valid position, look vector, up vector, fields of view and horizontal pixel resolution,
// construct a view plane in front of the camera and calculate a right vector
struct Camera setupCamera(struct Camera camera)
{
    struct Vector3 rightDirection = vectorCrossProduct(camera.lookDirection,camera.upDirection);
    camera.rightDirection = rightDirection; 

    struct ViewPlane viewPlane;
    // The look direction needs to be rotated to the four corners of the viewPlane
    // TODO: this method distorts the image so we've had to move the camera back and narrow its field of view.  Fix it, then tidy it up.
    struct Vector3 corner = camera.lookDirection;
    struct Vector3 right  = camera.rightDirection;
    corner = vectorRotate(corner, camera.upDirection, -camera.horizontalFieldOfView/2);
    right  = vectorCrossProduct(corner, camera.upDirection);
    corner = vectorRotate(corner, right, -camera.verticalFieldOfView/2);
    viewPlane.topLeft = vectorNormalise(corner);
    corner = camera.lookDirection;
    corner = vectorRotate(corner, camera.upDirection, camera.horizontalFieldOfView/2);
    right  = vectorCrossProduct(corner, camera.upDirection);
    corner = vectorRotate(corner, right, -camera.verticalFieldOfView/2);
    viewPlane.topRight = vectorNormalise(corner);
    corner = camera.lookDirection;
    corner = vectorRotate(corner, camera.upDirection, -camera.horizontalFieldOfView/2);
    right  = vectorCrossProduct(corner, camera.upDirection);
    corner = vectorRotate(corner, right, camera.verticalFieldOfView/2);
    viewPlane.bottomLeft = vectorNormalise(corner);
    corner = camera.lookDirection;
    corner = vectorRotate(corner, camera.upDirection, camera.horizontalFieldOfView/2);
    right  = vectorCrossProduct(corner, camera.upDirection);
    corner = vectorRotate(corner, right, camera.verticalFieldOfView/2);
    viewPlane.bottomRight = vectorNormalise(corner);

    camera.viewPlane = viewPlane;
    return camera;
}
