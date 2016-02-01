#include "spin1_api.h"
#include "spinn_io.h"
#include "spinn_sdp.h"
#include "arithmetic.h"
#include "geometry.h"
#include "scene.h"

#define SDP_HEADER_SIZE             (24) // bytes


int lower8  = (1<<8)-1;
int upper8  = ~((int)((1<<8)-1));
int multicastControlMask = ((1<<16)-1)^((1<<8)-1); // Only look at bits 15:8 for control
int proc001Key = (1<<10)+(1<<9); // key for packets to be routed to chip (0,0), processor 1 (e.g., pixel packets)

const uint firstByte  = (1<<8)-1;
const uint secondByte = (firstByte<<8);
const uint thirdByte  = (secondByte<<8);
const uint fourthByte = (thirdByte<<8);




// The maximum recursion depth for reflected/refracted rays
int maxDepth = 15;

// For a single ray, determine the nearest intersection point.
// If it hits an object, calculate diffuse colour and specular colour.
// If the surface is reflective, recursive call for a reflection ray.
// If the surface is transparent, recursive call for a reflection ray.
struct Vector3 traceRay(struct Ray ray, int depth, int reflectionFactor, 
                        struct Sphere* spheres, struct Light* lights,
                        int numberOfSpheres, int numberOfLights, int debug)
{
    struct Vector3 colour = {0,0,0};

    if (depth>maxDepth)
        return colour;

    // Specular colour is held seperately so transparency doesn't reduce
    // specular highlights
    struct Vector3 specularColour = {0,0,0};

    // Determine the closest intersecting sphere.
    int closestIntersection = 0;
    int intersectingObject  = -1;
    
    int i;  // Check intersection will all spheres
    for (i=0; i<numberOfSpheres; i++)
    {
        int sphereIntersect = sphereIntersection(spheres[i], ray);
        // Find nearest positive intersection
        if (sphereIntersect > 0 && (sphereIntersect < closestIntersection || closestIntersection==0))
        {
            closestIntersection = sphereIntersect;
            intersectingObject = i;
        }
    }

    // If there is an intersection
    if (intersectingObject != -1)
    {
        struct Vector3 intersectionPoint = \
            vectorAdd(ray.origin, vectorScalarMultiply(ray.direction, closestIntersection));

        struct Vector3 surfaceNormal = vectorNormalise(vectorSubtract(intersectionPoint, spheres[intersectingObject].position));
        struct Vector3 invertedSurfaceNormal = vectorInvert(surfaceNormal);

        // If the light is exiting a transparent object, do exit refraction, otherwise we've hit something...
        if (vectorMagnitude(vectorSubtract(ray.origin,spheres[intersectingObject].position)) < spheres[intersectingObject].radius)
        {
            // The ray came from inside a transparent sphere, do exit refraction
            struct Ray exitRefraction = {intersectionPoint, vectorRefract(ray.direction, surfaceNormal, spheres[intersectingObject].refractiveIndex, (1<<16))};
            exitRefraction = rayNudge(exitRefraction); // Nudge the ray along to stop it re-interacting with the same surface
            colour = traceRay(exitRefraction, depth+1, reflectionFactor, spheres, lights, numberOfSpheres, numberOfLights, debug);
        }
        else // We've hit an object
        {
            // Diffuse and specular light
            // Sum light contributions
            for (i=0; i<numberOfLights; i++)
            {
                int lightVisible = 1; // Occlusion check

                struct Vector3 shadowRayDirection   = vectorSubtract(lights[i].position, intersectionPoint);
                int shadowRayLength                 = vectorMagnitude(shadowRayDirection);
                shadowRayDirection                  = vectorNormalise(shadowRayDirection);
                struct Ray shadowRay = {intersectionPoint, shadowRayDirection};
                shadowRay = rayNudge(shadowRay); // Nudge the ray along to stop it re-interacting with the same surface

                struct Vector3 visibility = {1<<16,1<<16,1<<16}; // used to accumulate shadows from TRANSPARENT objects

                int j;
                for (j=0; j<numberOfSpheres; j++) // Check all objects for occlusion
                {
                    int shadowIntersect = sphereIntersection(spheres[j], shadowRay); // < 0.0 if there is no intersection, otherwise it is the length along the ray at which the intersection happens

                    if (shadowIntersect >= 0 && shadowIntersect < shadowRayLength ) // If the light is ocluded
                    {
                        if (vectorSum(spheres[j].transparency)<10) // If the occluding object is opaque, stop checking
                        {
                            lightVisible = 0; // Opaque object blocks light
                            break;
                        }
                        else
                        {
                            visibility = vectorFraction(visibility, spheres[j].transparency); // Transparent object blocks some light
                        }
                    }
                }
                if (lightVisible)
                {
                    // Diffuse term with Lambert's cosine law
                    int cosineLawTerm = vectorUnitDotProduct(surfaceNormal,shadowRayDirection);
                    cosineLawTerm = (cosineLawTerm < 0 ? -cosineLawTerm : cosineLawTerm);
                    colour = vectorAdd(colour, vectorScalarFraction(vectorFraction(vectorFraction(lights[i].colour,visibility), spheres[intersectingObject].colour),cosineLawTerm) );
                    
                    // Specular term with Phong term
                    struct Ray lightRay = {lights[i].position, vectorInvert(shadowRay.direction)};
                    struct Ray reflectedLightRay = {intersectionPoint, vectorSubtract(lightRay.direction , vectorScalarMultiply(surfaceNormal, 2*vectorUnitDotProduct(lightRay.direction,surfaceNormal))) };
                    reflectedLightRay = rayNudge(reflectedLightRay); // Nudge the ray along to stop it re-interacting with the same surface

                    int phongTerm   = vectorUnitDotProduct(ray.direction, reflectedLightRay.direction);
                    phongTerm       = (phongTerm < 0 ? -phongTerm : phongTerm); // having to get abs of value because dot product is probably using an inverted vector
                    phongTerm       = fp_frac(phongTerm,phongTerm); // Cheating to avoid doing exponentiation. p^2
                    phongTerm       = fp_frac(phongTerm,phongTerm); // p^4
                    phongTerm       = fp_frac(phongTerm,phongTerm); // p^8
                    phongTerm       = fp_frac(phongTerm,phongTerm); // p^16

                    specularColour = vectorAdd(specularColour, vectorFraction(vectorScalarFraction(vectorScalarFraction(vectorFraction(lights[i].colour,visibility), phongTerm), spheres[intersectingObject].shininess),vectorNormalise(spheres[intersectingObject].colour)));
                }
            }

            // Reflections - only do if we haven't hit the reflection limit
            if (spheres[intersectingObject].reflectivity > 0 && reflectionFactor > 10)
            {
                // Make a reflected ray, call this function on it
                struct Ray reflectedRay = {intersectionPoint, vectorSubtract(ray.direction, vectorScalarMultiply(surfaceNormal, 2*vectorUnitDotProduct(ray.direction,surfaceNormal))) };
                reflectedRay = rayNudge(reflectedRay); // Nudge the ray along to stop it re-interacting with the same surface
                colour = vectorAdd(vectorScalarFraction(colour,(1<<16)-spheres[intersectingObject].reflectivity), vectorScalarFraction(traceRay(reflectedRay, depth+1, fp_frac(reflectionFactor,spheres[intersectingObject].reflectivity), spheres, lights, numberOfSpheres, numberOfLights, debug),spheres[intersectingObject].reflectivity));
            }

            // Transparency / refraction - cast a refraction ray
            if (vectorSum(spheres[intersectingObject].transparency) > 3)
            {
                struct Vector3 opacity = {1<<16,1<<16,1<<16};
                opacity = vectorSubtract(opacity, spheres[intersectingObject].transparency);                

                // Do entry refraction
                struct Ray transmittedRay = {intersectionPoint, vectorRefract(ray.direction, invertedSurfaceNormal, (1<<16), spheres[intersectingObject].refractiveIndex)};
                transmittedRay = rayNudge(transmittedRay);
                colour = vectorAdd(vectorMultiply(colour,opacity), vectorFraction(traceRay(transmittedRay, depth+1, reflectionFactor, spheres, lights, numberOfSpheres, numberOfLights, debug), spheres[intersectingObject].transparency)  );
            }

            // Re-add in specular highlight here, so it's not reduced by transparency
            colour = vectorAdd(colour, specularColour);
        }

    }

    return vectorClamp(colour, 0, (255<<16));
}
// End of ray trace for single ray








// Light objects (position, colour)
struct Light lights[1] = {{{-(80<<9),80<<9,-(80<<9)},{250<<16,250<<16,250<<16}}};

// Sphere objects (position, radius, colour, reflectivity, transparency, refractive index, shininess)
struct Sphere spheres[8] = {    
                                {{60<<9,0,-(60<<9)},18<<9,{210<<8,200<<8,200<<8}, 154<<8, {0,0,0}, 1<<16, 1<<16} , // ball 1
                                {{30<<9,0,70<<9},18<<9,{1<<16,1<<16,1<<16}, 13<<8, {192<<8,192<<8,230<<8}, 282<<8, 1<<16} , // ball 2
                                {{-(10<<9),0,-(10<<9)},18<<9,{236<<8,128<<8,38<<8}, 26<<8, {0,0,0}, 1<<16, 128<<8} , // ball 3

                                {{160<<16,0,0},(160<<16) - (100<<9),{230<<8,230<<8,230<<8}, 0, {0,0,0}, 1<<16, 0} , 

                                {{0,0,160<<16},(160<<16) - (100<<9),{80<<8,80<<8,230<<8}, 0, {0,0,0}, 1<<16, 0} , 
                                {{0,0,-(160<<16)},(160<<16) - (100<<9),{230<<8,80<<8,80<<8}, 0, {0,0,0}, 1<<16, 0} , 

                                {{0,160<<16,0},(160<<16) - (180<<9),{230<<8,230<<8,230<<8}, 0, {0,0,0}, 1<<16, 0} , 
                                {{0,-(160<<16),0},(160<<16) - (20<<9),{230<<8,230<<8,230<<8}, 0, {0,0,0}, 1<<16, 0} , 

                           };

struct Vector3* horizontalInterpolations1;
struct Vector3* horizontalInterpolations2;







// Do the full ray trace.  NodeID is used to determine the first pixel we care about.  Number of nodes is used to determine how far we jump
void trace(int horizontalPixels, int verticalPixels, int horizontalFieldOfView, int verticalFieldOfView, int antialiasing, int x, int y, int z, int lookX, int lookY, int lookZ, int upX, int upY, int upZ, int nodeID, int numberOfNodes)
{
    int numberOfLights  = sizeof(lights)  / sizeof(struct Light);
    int numberOfSpheres = sizeof(spheres) / sizeof(struct Sphere);

    // Camera
    struct Camera camera = { {x,y,z}, {lookX,lookY,lookZ}, {upX,upY,upZ}, horizontalFieldOfView, verticalFieldOfView };
    camera.horizontalPixels = horizontalPixels;
    camera.verticalPixels   = verticalPixels;
    camera.antialiasing = antialiasing;
    camera = setupCamera(camera); // Sets up the right vector and the viewplane

    // For use in antialiasing
    int antialiasingStartPoint  = -(1<<16) + (1<<16)/camera.antialiasing; // antialiasing isn't bitshifted, so normal divide
    int antialiasingStepSize    = (2<<16)/camera.antialiasing;
    // For repeated use in interpolation
    struct Vector3 topRightMinusTopLeft = vectorSubtract(camera.viewPlane.topRight, camera.viewPlane.topLeft);
    struct Vector3 bottomRightMinusBottomLeft = vectorSubtract(camera.viewPlane.bottomRight, camera.viewPlane.bottomLeft);

    int i,j;
    // Determine the first pixel this node cares about based on its ID
    int iFirst = 0;
    int jFirst = nodeID;
    while (jFirst >= camera.verticalPixels)
        jFirst -= camera.verticalPixels;
        iFirst += 1;
    int overshoot = jFirst;

    // Main loop
    for (i=iFirst; i<camera.horizontalPixels; i++)
    {
        uint iSegment1 = ((i&upper8)>>8); // for MC transmission
        uint iSegment2 = ((i&lower8)<<24);

        // This gets a little messy because of the antialiasing, but we're really just iterating over sub-pixels, casting rays, and averaging over pixels
        int k;
        for (k=0; k<camera.antialiasing; k++)
        {   
            horizontalInterpolations1[k] = vectorAdd(vectorScalarMultiply(topRightMinusTopLeft, ((i<<16)+(antialiasingStartPoint+antialiasingStepSize*k))/camera.horizontalPixels), camera.viewPlane.topLeft);
            horizontalInterpolations2[k] = vectorAdd(vectorScalarMultiply(bottomRightMinusBottomLeft, ((i<<16)+(antialiasingStartPoint+antialiasingStepSize*k))/camera.horizontalPixels), camera.viewPlane.bottomLeft);
        }

        j = overshoot;
        while (j<camera.verticalPixels)
        {
            //io_printf(IO_STD, "j %d\n", j);
            struct Vector3 accumulatedColour = {0,0,0}; // to accumulate colours from sub-pixels (antialiasing)

            for (k=0; k<camera.antialiasing; k++)
            {
                // For repeated use in vertical interpolation
                struct Vector3 horizontal2MinusHorizontal1 = vectorSubtract(horizontalInterpolations2[k] , horizontalInterpolations1[k]);

                int l; // vertical antialiasing
                for (l=0; l<camera.antialiasing; l++)
                {
                    struct Vector3 rayDirection = vectorNormalise(vectorAdd(vectorScalarMultiply(horizontal2MinusHorizontal1, ((j<<16)+(antialiasingStartPoint+antialiasingStepSize*l))/camera.verticalPixels), horizontalInterpolations1[k]));
                    struct Ray ray = {camera.position, rayDirection};
                    accumulatedColour = vectorAdd(accumulatedColour, traceRay(ray, 0, (1<<16), spheres, lights, numberOfSpheres, numberOfLights, 0));
                }
            }

            struct Vector3 colour = vectorScalarFraction(vectorScalarFraction(accumulatedColour, (1<<16)/(camera.antialiasing*camera.antialiasing)),1);

            uint key        = (j<<16) | proc001Key | iSegment1;
            uint payload    = iSegment2 | (((uint)colour.x)<<16) | (((uint)colour.y)<<8) | ((uint)colour.z);

            spin1_send_mc_packet(key,payload,1);

        // End of inner loop
        j+=numberOfNodes;
        if (j >= camera.verticalPixels)
            overshoot = j - (j/camera.verticalPixels)*camera.verticalPixels;

        }
    }

}

// end of tracer.c




void sdp_packet_callback(uint msg, uint port)
{
    sdp_msg_t *sdp_msg = (sdp_msg_t *) msg;

    if (sdp_msg->cmd_rc == 4) // if it's a trace message   
    {
        int camx = 0, camy = 0, camz = 0, lookx = 0, looky = 0, lookz = 0, upx = 0, upy = 0, upz = 0, width = 0, height = 0, hField = 0, vField = 0, antialiasing = 0, nodeID=0, numberOfNodes=16;

        int i;
        for (i=0; i<(sdp_msg->length-24)/4; i++)
        {
            int var = 0;
            var += (sdp_msg->data[i*4]<<0);
            var += (sdp_msg->data[i*4+1]<<8);
            var += (sdp_msg->data[i*4+2]<<16);
            var += (sdp_msg->data[i*4+3]<<24);      

            switch(i)
            {
                case 0: camx = var; break;
                case 1: camy = var; break;
                case 2: camz = var; break;
                case 3: lookx = var; break;
                case 4: looky = var; break;
                case 5: lookz = var; break;
                case 6: upx = var; break;
                case 7: upy = var; break;
                case 8: upz = var; break;
                case 9: width = var; break;
                case 10: height = var; break;
                case 11: hField = var; break;
                case 12: vField = var; break;
                case 13: antialiasing = var; break;
                case 14: nodeID = var; break;
                case 15: numberOfNodes = var; break;
            }
        }

        trace(width, height, hField, vField, antialiasing, camx, camy, camz, lookx, looky, lookz, upx, upy, upz, nodeID, numberOfNodes);
    }

    spin1_msg_free(sdp_msg);
}


void mc_packet_callback(uint key, uint payload)
{
    io_printf(IO_STD, "MC packet received.\n");
}

// Everybody is doing it as we don't know which cores will be available to the simulation // FIXME use a smarter solution
void load_mc_routing_tables()
{
    uint x = ((spin1_get_chip_id()&upper8)>>8);
    uint y = spin1_get_chip_id()&lower8;


    // Routing table entry for sending pixel packets to 0, 0, 1
    uint destination;    
    if (x>0 && y>0)
        destination = (1<<4); // south-west
    else if (x>0 && y==0)
        destination = (1<<5); // south
    else if (x==0 && y>0)
        destination = 1; // west
    else
        destination = (1<<7); // processor 1

    spin1_set_mc_table_entry(0, proc001Key, multicastControlMask, destination);

    /*int i=1;
    int xDest,yDest,proc;
    for (xDest=0; xDest<10; xDest++)
    {
        for (yDest=0; yDest<10; yDest++)
        {
            for (proc=1; proc<17; proc++)
            {
                if(x==xDest && y==yDest)
                    destination = (1<<(proc+6));
                else if(x>xDest && y==yDest)
                    destination = (1<<5);
                else if(x==xDest && y>yDest)
                    destination = 1;
                else if(x>xDest && y>yDest)
                    destination = (1<<4);
                else if (x<xDest && y==yDest)
                    destination = (1<<2);
                else if (x==xDest && y<yDest)
                    destination = (1<<3);
                else if (x<xDest && y<yDest)
                    destination = (1<<1);

                spin1_set_mc_table_entry(i, (xDest<<24)|(yDest<<16)|(proc), ~multicastControlMask, destination);
                i++;
            }
        }
    }*/

}

void c_main()
{   
    load_mc_routing_tables();

    horizontalInterpolations1 = spin1_malloc(10 * sizeof(struct Vector3));
    horizontalInterpolations2 = spin1_malloc(10 * sizeof(struct Vector3));

    spin1_callback_on(SDP_PACKET_RX, sdp_packet_callback, 2);
    spin1_callback_on(MC_PACKET_RECEIVED, mc_packet_callback, 1);

    spin1_start();
}
