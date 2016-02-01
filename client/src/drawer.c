#include <stdio.h>
#include <time.h>
#include <math.h>
#include <GL/freeglut.h>
#include "listener.h"



struct Vector3
{      
    float x, y, z;
};

struct Vector3 position = {-220.0, 50.0, 0.0};
struct Vector3 look = {1.0, 0.0, 0.0};
struct Vector3 up = {0.0, 1.0, 0.0};

int moving = 0;
int strafing = 0;
int turningLeftRight = 0;
int turningUpDown = 0;
int rolling = 0;

float moveAmount = 0.00003;
float turnAmount = 0.0000003;

float verticalFieldOfView = 50.0;
float horizontalFieldOfView = 60.0;

void display(void)
{ /* Called every time OpenGL needs to update the display */

    glClearColor (1.0,1.0,1.0,0.001);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glDrawPixels(frameWidth, frameHeight, GL_RGB, GL_UNSIGNED_BYTE, viewingFrame);

    glutSwapBuffers();
}

void reshape(int width, int height)
{
	frameWidth = width;
	frameHeight = height;
	glViewport(0, 0, (GLsizei) width, (GLsizei) height);
	glLoadIdentity();
}

void special(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_UP:
		turningUpDown=-1;
		break;
	case GLUT_KEY_DOWN:
		turningUpDown=1;
		break;
	case GLUT_KEY_RIGHT:
		rolling=-1;
		break;
	case GLUT_KEY_LEFT:
		rolling=1;
		break;
	}
}

void specialUp(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_UP:
		turningUpDown=0;
		break;
	case GLUT_KEY_DOWN:
		turningUpDown=0;
		break;
	case GLUT_KEY_RIGHT:
		rolling=0;
		break;
	case GLUT_KEY_LEFT:
		rolling=0;
		break;
	}
}

void keyPressed (unsigned char key, int x, int y)
{
	switch(key)
	{
	case 'w':
		moving=1;
		break;
	case 's':
		moving=-1;
		break;
	case 'a':
		turningLeftRight=-1;
		break;
	case 'd':
		turningLeftRight=1;
		break;
	case 'q':
		strafing=1;
		break;
	case 'e':
		strafing=-1;
		break;
	}
}

void keyReleased (unsigned char key, int x, int y)
{
	switch(key)
	{
	case 'w':
		moving=0;
		break;
	case 's':
		moving=0;
		break;
	case 'a':
		turningLeftRight=0;
		break;
	case 'd':
		turningLeftRight=0;
		break;
	case 'q':
		strafing=0;
		break;
	case 'e':
		strafing=0;
		break;
	}
}






struct Vector3 fVectorNormalise(struct Vector3 in)
{
    float magnitudeReciprocal = 1.0/sqrt(in.x*in.x+in.y*in.y+in.z*in.z);
    struct Vector3 result = {in.x*magnitudeReciprocal, in.y*magnitudeReciprocal, in.z*magnitudeReciprocal};
    return result;
}

struct Vector3 fVectorCrossProduct(struct Vector3 a, struct Vector3 b)
{
    struct Vector3 result = {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
    return result;
}

// Rotate the first vector around the second
struct Vector3 fVectorRotate(struct Vector3 rotated, struct Vector3 rotateAbout, float amount)
{
    float c = cos(amount);
    float s = sin(amount);
    float t = 1-cos(amount);

    struct Vector3 result;
    result.x = rotated.x*(t*rotateAbout.x*rotateAbout.x+c) + \
        rotated.y*(t*rotateAbout.x*rotateAbout.y+s*rotateAbout.z) + \
        rotated.z*(t*rotateAbout.x*rotateAbout.z-s*rotateAbout.y);
    result.y = rotated.x*(t*rotateAbout.x*rotateAbout.y-s*rotateAbout.z) + \
        rotated.y*(t*rotateAbout.y*rotateAbout.y+c) + \
        rotated.z*(t*rotateAbout.y*rotateAbout.z+s*rotateAbout.x);
    result.z = rotated.x*(t*rotateAbout.z*rotateAbout.x+s*rotateAbout.y) + \
        rotated.y*(t*rotateAbout.y*rotateAbout.z-s*rotateAbout.x) + \
        rotated.z*(t*rotateAbout.z*rotateAbout.z+c);

    result = fVectorNormalise(result);
    return result;
}






void calculateMovement(int timestep)
{
    // Forward movement
    position.x += look.x*timestep*moveAmount*moving;
    position.y += look.y*timestep*moveAmount*moving;
    position.z += look.z*timestep*moveAmount*moving;

    struct Vector3 right = fVectorCrossProduct(up,look);

    // Strafing movement
    position.x += right.x*timestep*moveAmount*strafing;
    position.y += right.y*timestep*moveAmount*strafing;
    position.z += right.z*timestep*moveAmount*strafing;

    // To turn left/right, rotate the look vector around the up vector
    look = fVectorRotate(look, up, timestep*turnAmount*turningLeftRight);

    // To turn up/down, rotate the look vector and up vector about the right vector
    look = fVectorRotate(look, right, timestep*turnAmount*turningUpDown);
    up = fVectorRotate(up, right, timestep*turnAmount*turningUpDown);

    // To roll, rotate the up vector around the look vector
    up = fVectorRotate(up, look, timestep*turnAmount*rolling);
}

clock_t prevTime;
clock_t currTime;


struct node
{      
    int x, y, proc;
};

struct node *nodes;
int numberOfNodes;


int done1 = 0;
int done2 = 0;

void updateImage()
{
    int i;

    for (i=0; i<numberOfNodes; i++)
    {

        send_trace_trigger((int)(position.x*powf(2.0,9.0)),(int)(position.y*powf(2.0,9.0)),(int)(position.z*powf(2.0,9.0)),(int)(look.x*powf(2.0,16.0)),(int)(look.y*powf(2.0,16.0)),(int)(look.z*powf(2.0,16.0)),(int)(up.x*powf(2.0,16.0)),(int)(up.y*powf(2.0,16.0)),(int)(up.z*powf(2.0,16.0)),frameWidth,frameHeight,(int)(horizontalFieldOfView*powf(2.0,16.0)),(int)(verticalFieldOfView*powf(2.0,16.0)),antialiasing, nodes[i].x, nodes[i].y, nodes[i].proc, i, numberOfNodes);
    }
}

int swappedBuffers = 0;

void idleFunctionLoop()
{
	//reshape(frameWidth,frameHeight);

    if (currTime-lastMessageTime>=30000 && !swappedBuffers && frameBuffering)
    {
        unsigned char *temp = viewingFrame;
        viewingFrame = drawingFrame;
        drawingFrame = temp;
        swappedBuffers = 1;
    }
    if (currTime-lastMessageTime<=20000 && swappedBuffers)
    {
        swappedBuffers = 0;
    }

    currTime=clock();
    // Calculate movement ten times a second
	if(currTime>prevTime+10000)
	{
        //printf("%d\n", currTime-lastMessageTime);

        int timeSinceLastUpdate = currTime-prevTime;
        calculateMovement(timeSinceLastUpdate);

		prevTime=currTime;
        display();
	}
} 

int main(int argc, char **argv)
{
    lastMessageTime = clock();
    pthread_t p1;				// this sets up the thread that can come back to here from type

	init_udp_server_spinnaker();		//initialization of the port for receiving SpiNNaker frames

	pthread_create (&p1, NULL, input_thread, NULL);	// away it goes

    frameHeight     = (argc>1 ? atoi(argv[1]) : 256);
    frameWidth      = (int)((((int)horizontalFieldOfView*frameHeight))/verticalFieldOfView);
    antialiasing    = (argc>2 ? atoi(argv[2]) : 2);
    int xChips          = (argc>3 ? atoi(argv[3]) : 1);
    int yChips          = (argc>4 ? atoi(argv[4]) : 1);
    int procs           = (argc>5 ? atoi(argv[5]) : 16);
    frameBuffering      = (argc>6 ? atoi(argv[6]) : 0);

    numberOfNodes = xChips*yChips*procs - 1;
    nodes = (struct node*)malloc(numberOfNodes*sizeof(struct node));
    int i;
    int x = 0;
    int y = 0;
    int proc = 2; // 0,0,1 is the aggregator
    for(i=0; i<numberOfNodes; i++)
    {
        nodes[i].x = x;
        nodes[i].y = y;
        nodes[i].proc = proc;
        proc++;
        if (proc > procs)
        {
            proc = 1;
            y++;
            if (y >= yChips)
            {
                y=0;
                x++;
                if (x >= xChips)
                {
                    x=0;
                }
            }
        }
    }

    prevTime = clock();
    drawingFrame = malloc(sizeof(char)*frameWidth*frameHeight*3);
    viewingFrame = malloc(sizeof(char)*frameWidth*frameHeight*3);

	glutInit(&argc, argv);                /* Initialise OpenGL */
	glutInitDisplayMode (GLUT_DOUBLE);    /* Set the display mode */
	glutInitWindowSize (frameWidth,frameHeight);         /* Set the window size */
	glutInitWindowPosition (0, 100);    /* Set the window position */
	glutCreateWindow ("Ray Tracer");  /* Create the window */

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	glutDisplayFunc(display);             /* Register the "display" function */
	glutReshapeFunc(reshape);             /* Register the "reshape" function */
	glutSpecialFunc (special);
	glutSpecialUpFunc (specialUp);
	glutKeyboardFunc(keyPressed);
	glutKeyboardUpFunc(keyReleased);
	glutIdleFunc(idleFunctionLoop);

	glutMainLoop(); /* Enter the main OpenGL loop */

    free(drawingFrame);
    free(viewingFrame);

	return 0;
}
