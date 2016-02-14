// Jason Rohrer
// RadiosGLView.cpp

/**
*
*	OpenGL viewer for RadiosGL rendered scenes
*
*	Created 1-14-2000
*	Mods:
*		Jason Rohrer	1-15-2000		Added timer function to make keyboard
*										interaction smoother and more intuitive
*
*		Jason Rohrer	1-18-2000		Moved SetMouse call from within mouse handler
*										to inside re-display function
*										SetMouse was posting a new mouse event too
*										quickly, putting the delta-angles back to 0
*										before the posted redisplay happened.
*
*										This made view motion jerky on the PC
*										Fixed it by only centering mouse when it leaves window
*										(or hits window border)
*										Added mouse capture to window on PC so that
*										mouse events outside window border are captured
*								
*										Added "o" key control to switch into fullscreen mode
*/


#define APP_NAME			"RadiosGLView"
#define SETTINGS_FILE_NAME	"RadiosGLView.ini"		

#define FORWARD_MOTION_TIMER 200



#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "gl.h"
#include "glut.h"

#include "RadiosityConstants.h"
#include "Vertex.h"
#include "Dither.h"

#include "UniversalFileIO.h"

// function for force-setting mouse position
#include "SetMouse.h"


// prototypes
void cleanup();
int loadSettings();
void initModel();
void glutMotion(int x, int y);
void draw (void);
void glutDisplay (void);
void glutResize (int w, int h);
void glutKeyboard (unsigned char key, int x, int y);
void glInit (void);
void glutTimer( int value );
void glutIdle( void );
void glutEntry( int state );


// vector
typedef float vec3_t[3]; 


// these are loaded from the ini file
	int screenCenterX;
	int screenCenterY;

	float invScreenCenterX;
	float invScreenCenterY;

	int dithering;

	int invMouse;

	int fullscreen;
	char *modelFileName = new char[99];

	// window width and height
	int winW;
	int winH;


long numPatches;
Vertex *triangles[3];
Vertex *textAnchors[3];
int startTextID = 13;


float translationX = 0;
float translationY = 0;
float translationZ = 0;

// position for view
float positionX = 0;
float positionY = 25;
float positionZ = 0;


float speed = 0.5;		// speed of motion
float rotationSpeed = 1.0;	// speed of view rotation

// two vectors specifying view direction
float lookX = 0;
float lookY = 0;
float lookZ = 1;

float upX = 0;
float upY = 1;
float upZ = 0;

// vector pointing to left to help with rotations
float leftX = -1;
float leftY = 0;
float leftZ = 0;

// change in position
float deltaPosition = 0;

// direction of motion
char forwardMode = false;
char backwardMode = false;


// is mouse engaged for view angle movement
char mouseEngaged = false;

int lastMouseX = screenCenterX;
int lastMouseY = screenCenterY;

// set to true during recentering of mouse
char ignoreEvent = false;

// amount to rotate about axis 
float rotate = 0.0f;

// vector which describes the axis to rotate about 
vec3_t axis = {1.0, 0.0, 0.0};

// global delta rotation, for use with the mouse 
vec3_t gRot = {0,0,0};

// global total rotation
vec3_t gRotNet = {0,0,0};



FILE *logFile;

void draw (void) {
	
	glPushMatrix ();

	glRotatef (-rotate, axis[0], axis[1], axis[2]);

	glTranslatef (-15.0, 10.0, 5.0);

	//	draws radiosity polygons		

	int textID = startTextID;

	for( int p=0; p<numPatches; p++ ) {	// for each patch 
		glEnable( GL_TEXTURE_2D );
		glBindTexture(GL_TEXTURE_2D, textID );
		
		if( glGetError() != GL_NO_ERROR ) {		// error
			fprintf( logFile, "error binding texture\n");
			}
			
	   	glColor4f(1.0, 1.0, 1.0, 1.0);
		
		glBegin(GL_TRIANGLES);
			for( int v=0; v<3; v++ ) {
				glTexCoord2f ( textAnchors[v][p].x, textAnchors[v][p].y );
	    		glVertex3f( triangles[v][p].x, triangles[v][p].y, triangles[v][p].z);
	    		}
		glEnd();
		
		glDisable( GL_TEXTURE_2D );
		
		textID++;	// move on to next texture
		}
	glPopMatrix ();
	}



// save last orientation in case of a redraw with no change
float lastPositionX = positionX; 
float lastPostionY = positionY;
float lastPostionZ = positionZ;
float lastUpX = upX;
float lastUpY = upY;
float lastUpZ = upZ;
float lastLookX = lookX;
float lastLookY = lookY;
float lastLookZ = lookZ;


// called when a redisplay is posted
void glutDisplay (void) {
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	float delZ = 0.0;

	// if moving foward
	if( forwardMode ) {
		deltaPosition = speed;
		}
	else if( backwardMode ) {
		deltaPosition = -speed;
		}
	else {
		deltaPosition = 0.0;
		}	

	// keep view position in a plane above the floor:  don't allow changes in y position
	// positionY = lookY*deltaPosition + positionY;
	
	// need to normalize lookX and lookZ, since when looking up, they both might be 0
	float invLengthXZ = 1.0 / sqrt(lookX*lookX + lookZ*lookZ);
	positionX = lookX * invLengthXZ * deltaPosition + positionX;
	positionZ = lookZ * invLengthXZ * deltaPosition + positionZ;


	glMatrixMode (GL_PROJECTION);

	glLoadIdentity ();

	gluPerspective (90, winW / winH, 1, 9999);

	if( gRot[0] == 0 && gRot[1] == 0 ) {	// no change in view orientation
		upX = lastUpX;
		upY = lastUpY;
		upZ = lastUpZ;
		lookX = lastLookX;
		lookY = lastLookY;
		lookZ = lastLookZ;
		}
	else {		// new orientation

		// first, rotate up-down (pitch)
		// only do this rotation if it doesn't push look past straight up or straight down
		if( gRotNet[0] + gRot[0] > -halfPI && gRotNet[0] + gRot[0] < halfPI ) {
			float oldUpX = upX;
			float oldUpY = upY;
			float oldUpZ = upZ;

			upX += lookX * gRot[0];
			upY += lookY * gRot[0];
			upZ += lookZ * gRot[0];

			// normalize up
			float invLength = 1.0 / sqrt(upX*upX + upY*upY + upZ*upZ);
			upX = upX * invLength;
			upY = upY * invLength;
			upZ = upZ * invLength;


			lookX += oldUpX * -gRot[0];
			lookY += oldUpY * -gRot[0];
			lookZ += oldUpZ * -gRot[0];

			// normalize look
			invLength = 1.0 / sqrt(lookX*lookX + lookY*lookY + lookZ*lookZ);
			lookX *= invLength;
			lookY *= invLength;
			lookZ *= invLength;
			
			gRotNet[0] += gRot[0];
			}

		// now rotate left-right
		// this should by done by rotating all three vectors around y-axis
		// only X and Z components will change

		float cosTheta = cos( gRot[1] );
		float sinTheta = sin( gRot[1] );

		// first, look direction
		float oldLookX = lookX;
		float oldLookZ = lookZ;

		lookX = cosTheta * oldLookX + sinTheta * oldLookZ;
		lookZ = -sinTheta * oldLookX + cosTheta * oldLookZ;

		// then, up direction
		float oldUpX = upX;
		float oldUpZ = upZ;

		upX = cosTheta * oldUpX + sinTheta * oldUpZ;
		upZ = -sinTheta * oldUpX + cosTheta * oldUpZ;
		
		gRotNet[1] += gRot[1];
		
		lastUpX = upX;
		lastUpY = upY;
		lastUpZ = upZ;
		lastLookX = lookX;
		lastLookY = lookY;
		lastLookZ = lookZ;
		}
		
		
	gluLookAt( positionX, positionY, positionZ,
	           positionX + lookX, positionY + lookY, positionZ + lookZ,
	           upX, upY, upZ );

	draw ();

	glutSwapBuffers();
	}


void glutIdle( void ) {
	glutPostRedisplay();
	}


char timing = false;	// are we timing interval between key strokes?
char keyPress = false;		// are we in the middle of a key-down session?

void glutTimer( int value ) {
	
	// NOTE:  This seems to rely on keyboard repeat speed....
	// would like to find a better method of using keyboard to move
	//		forward and backward in a 3d environment
	// Want a key-up, key-down event handler, but this is not available in GLUT!!!
	
	if( !timing && (backwardMode || forwardMode) ) {
		timing = true;	// wait one more timer cycle
		glutTimerFunc(FORWARD_MOTION_TIMER, glutTimer, 1);		// reset timer
		return;
		}
		
	if( timing ) {		// expire timers and current motion
		if( backwardMode ) {
			backwardMode = false;
			}
		if( forwardMode ) {
			forwardMode = false;
			}
		timing = false;
		keyPress = false;
		}
	glutPostRedisplay();
	}

// called on app start and on resize.
void glutResize (int w, int h) {	
	winW = w;
	winH = h;


	screenCenterX = winW / 2;
	screenCenterY = winH / 2;

	invScreenCenterX = 1/ (float)screenCenterX;
	invScreenCenterY = 1/ (float)screenCenterY;

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glViewport( 0, 0, winW, winH );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	gluPerspective( 90, winW / winH, 1, 9999 );

	glutPostRedisplay();
	}


// Keyboard handler
void glutKeyboard (unsigned char key, int x, int y) {	
	switch (key){
	    case 'q':
	    case 'Q':
	    	exit (1);
	    break;
	    case 'o':
	    case 'O':
	    	glutFullScreen();
	    break;
	    case 'e':
	    case 'E':
			// move forward
	 		forwardMode = true;
	 		backwardMode = false;
	 		
	 		if( !keyPress ) {	// start of a new key down
	 			keyPress = true;
	 			glutTimerFunc(FORWARD_MOTION_TIMER, glutTimer, 1);		// set timer
	 			}
			timing = false;		// reset key timer
	    break;
	    case 'd':
	    case 'D':
	    	// move backward
	    	//translationZ -= 1;
	    	forwardMode = false;
	 		backwardMode = true;
	 		
	 		if( !keyPress ) {	// start of a new key down
	 			keyPress = true;
	 			glutTimerFunc(FORWARD_MOTION_TIMER, glutTimer, 1);		// set timer
	 			}
			timing = false;		// reset key timer
	    break;
	    case 'a':
	    case 'A':
	    	// increase speed
	    	speed += 0.1;
	    break;
	    case 's':
	    case 'S':
	    	// decrease speed
	    	if( speed >= 0.1 ) speed -= 0.1;
	    	else speed = 0;
	    break;
		case 'z':
	    case 'Z':
	    	// increase rotation speed
	    	rotationSpeed += 0.1;
	    break;
	    case 'x':
	    case 'X':
	    	// decrease rotation speed
	    	if( rotationSpeed >= 0.1 ) rotationSpeed -= 0.1;
	    	else rotationSpeed = 0;
	    break;
	    case 'm':
	    case 'M':
	    	if( mouseEngaged ) {
	    		glutSetCursor( GLUT_CURSOR_LEFT_ARROW );	// mouse back on to normal mode
	    		mouseEngaged = false;
	    		ReleaseMouse();
	    		// stop rotation on mouse disengage
	    		gRot[0] = 0;
				gRot[1] = 0;
	    		}
	    	else {
	    		glutSetCursor( GLUT_CURSOR_NONE );	// cursor off, mouse in view control mode
	    		mouseEngaged = true;
	    		ignoreEvent = true;
	    		CaptureMouse();
	    		SetMouse( screenCenterX, screenCenterY );
	    		}
	    break;
	    break;
		}
    	glutPostRedisplay ();
	}




// Called when the mouse moves in our app area. 
void glutMotion(int x, int y) {
	
	if( mouseEngaged ) {	// only change view if mouse in view mode
	
		if( ignoreEvent ) {
			lastMouseX = x;
			lastMouseY = y;
			// make sure mouse back in screen area
			if( x < winW-5 && x > 5 && y < winH-5 && y > 5 ) {
				ignoreEvent = false;
				}
			else {		// try centering again, user might be fiesty
				SetMouse( screenCenterX, screenCenterY );
				}
			return;
			}
	
		gRot[0] = (lastMouseY - y)*rotationSpeed * invScreenCenterY * invMouse;
		gRot[1] = (lastMouseX - x)*rotationSpeed * invScreenCenterX;
		
		lastMouseX = x;
		lastMouseY = y;
		
		glutPostRedisplay ();

	 	
	 	// watch for mouse hitting edges of screen
	 	if( x >= winW-5 || x <= 5 || y >= winH-5 || y <= 5 ) {
	 		ignoreEvent = true;
	 		SetMouse( screenCenterX, screenCenterY );
	 		}
	 	}
	}

void glutEntry( int state ) {
	// reign mouse in if it leaves window while engaged
	if( state == GLUT_LEFT && mouseEngaged ) {
		ignoreEvent = true;
	 	SetMouse( screenCenterX, screenCenterY );
	 	}
	}


// Sets up various OpenGL stuff.
void glInit (void) {
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);
	}




int main (void) {
	
	logFile = fopen("RadiosGLView.log", "w");
	 
	if( loadSettings() != 0 ) {
		fprintf( logFile, "Settings file %s failed to open\n", SETTINGS_FILE_NAME );
		cleanup();
		return 2;		// settings didn't load
		}
	
	if( fullscreen ) {
		glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH |GLUT_FULLSCREEN );
		glutFullScreen();
		}
	else {
		glutInitDisplayMode (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH |GLUT_MULTISAMPLE);
		}

	glutInitWindowSize( winW, winH );
	glutCreateWindow( APP_NAME );
	glutKeyboardFunc( glutKeyboard );
	glutDisplayFunc( glutDisplay );
	glutReshapeFunc( glutResize );
	glutPassiveMotionFunc( glutMotion );
	glutIdleFunc( glutIdle );
	glutEntryFunc( glutEntry );


	glInit ();

	initModel();

	glutSetCursor( GLUT_CURSOR_NONE );	// cursor off, mouse in view control mode  		
	mouseEngaged = true;
	ignoreEvent = true;
	CaptureMouse();
	SetMouse( screenCenterX, screenCenterY );


	glutMainLoop( );
	
	cleanup();
	}


void cleanup() {
	fclose(logFile);
	delete [] modelFileName;
	}



int loadSettings() {
	
	FILE *settingsFile = fopen( SETTINGS_FILE_NAME, "r" );
	
	if( settingsFile == NULL ) {
		return -1;
		}
	
	char *stringBuffer = new char[99];
	
	// assumes that all parameters are in a specific order
	
	// read FULLSCREEN setting
	fscanf( settingsFile, "%s", stringBuffer );
	fscanf( settingsFile, "%d", &fullscreen );
	
	// read DITHER setting
	fscanf( settingsFile, "%s", stringBuffer );
	fscanf( settingsFile, "%d", &dithering );
	
	// read SCREENWIDE setting
	fscanf( settingsFile, "%s", stringBuffer );
	fscanf( settingsFile, "%d", &winW );
	
	// read SCREENHIGH setting
	fscanf( settingsFile, "%s", stringBuffer );
	fscanf( settingsFile, "%d", &winH );
		
	screenCenterX = winW / 2;
	screenCenterY = winH / 2;

	invScreenCenterX = 1/ (float)screenCenterX;
	invScreenCenterY = 1/ (float)screenCenterY;

	// read MODELFILE setting
	fscanf( settingsFile, "%s", stringBuffer );
	fscanf( settingsFile, "%s", modelFileName );
	
	// read INVERTMOUSE setting
	fscanf( settingsFile, "%s", stringBuffer );
	fscanf( settingsFile, "%d", &invMouse );
	
	// convert 0/1 switch to -1/1 needed for inverting mouse
	if( invMouse == 0 ) invMouse = 1;
	else invMouse = -1;

	fclose( settingsFile );
	return 0;
	}




void initModel() {

	UniversalFileIO fileIO;

	FILE *sceneFile = fopen( modelFileName, "rb" );
	
	if( sceneFile != NULL ) {
		fprintf( logFile, "Scene file opened.\n"); 
		}
	else {
		return;
		}

	numPatches = fileIO.freadLong( sceneFile );

	fprintf( logFile, "num patches = %d\n", numPatches);
	
	for( int v=0; v<3; v++ ) {
		triangles[v] = new Vertex[numPatches];
		textAnchors[v] = new Vertex[numPatches];
		}
	
	int textID = startTextID;	
	
	for( int p=0; p<numPatches; p++ ) {
		
		// read vertices for this patch
		for( int v=0; v<3; v++ ) {
			float x, y, z;
			
			
			x = fileIO.freadFloat( sceneFile );
			y = fileIO.freadFloat( sceneFile );
			z = fileIO.freadFloat( sceneFile );
			
			triangles[v][p].x = x;
			triangles[v][p].y = y;
			triangles[v][p].z = z;
			fprintf( logFile, "patch vert = (%f, %f, %f)\n", x,y,z);
			}
		
		// read texture dimensions	
		long textWide, textHigh;
		
		textWide = fileIO.freadLong( sceneFile );
		textHigh = fileIO.freadLong( sceneFile );

		// read texture anchors
		for( int v=0; v<3; v++ ) {
			long x, y;
			
			x = fileIO.freadLong( sceneFile );
			y = fileIO.freadLong( sceneFile );

			textAnchors[v][p].x = (float)x / (float)textWide;
			textAnchors[v][p].y = (float)y / (float)textHigh;
			fprintf( logFile, "texture anchors = (%f, %f)\n", textAnchors[v][p].x, textAnchors[v][p].y);
			}
		
		
		// read texture data
		long numBytes = textHigh * textWide * 4;

		
		fprintf( logFile, "textHigh, textWide = %d, %d\n", textHigh, textWide);
		
		unsigned char *rgba = new unsigned char[numBytes];
		
		fread ((void*)rgba, sizeof (unsigned char), numBytes, sceneFile); 

		unsigned char *argb16 = new unsigned char[ numBytes ];	// 16 bit dithered version
		
		if( dithering ) {	
			Dither16( (unsigned long*)rgba, textHigh, textWide, (unsigned long*)argb16, 1);
			}
			
		// convert incoming ARGB to RGBA
    	for ( int i=0; i<numBytes ; i+=4 ) {
        	int temp = rgba[i];
	       	rgba[i] = rgba[i + 1];
        	rgba[i + 1] = rgba[i + 2];
        	rgba[i + 2] = rgba[i + 3];
        	rgba[i + 3] = temp;
    		}

		

		// upload texture to OpenGL
		GLenum texFormat = GL_RGBA;
		GLenum internalTexFormat = GL_RGB16;
		
		glBindTexture (GL_TEXTURE_2D, textID);
    	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		if( dithering ) {
			glTexImage2D (GL_TEXTURE_2D, 0, internalTexFormat, textWide, textHigh, 0, texFormat, GL_UNSIGNED_BYTE, rgba);
			}
		else {
			glTexImage2D (GL_TEXTURE_2D, 0, texFormat, textWide, textHigh, 0, texFormat, GL_UNSIGNED_BYTE, rgba);
			}
			
		fprintf( logFile, "texture data read successfully\n");
    	// release data, its been uploaded 
  	  	delete [] rgba;
  	  	delete [] argb16;
  	  	
  	  	
  	  	textID++;	// move on to next texture
  	  	
  	  	}	// end for each patch
	}	// end initModel()

