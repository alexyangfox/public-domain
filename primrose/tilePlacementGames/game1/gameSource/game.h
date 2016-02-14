
// interface for game engine

// called by GLUT or iPhone app wrapper


void initFrameDrawer( int inWidth, int inHeight );


// called at application termination
// good time to save state for next launch
void freeFrameDrawer();


// draw scene into frame using GL function calls
void drawFrame();


// start of pointer press
void pointerDown( float inX, float inY );

// movement with pointer pressed
void pointerMove( float inX, float inY );

// end of pointer press
void pointerUp( float inX, float inY );



#include <stdint.h>
typedef int16_t Sint16;
typedef uint8_t Uint8;

// sample rate shared by game engine and sound rendering platform
//#define gameSoundSampleRate 22050
//#define gameSoundSampleRate 44100
#define gameSoundSampleRate 11025

// gets the next buffer-full of sound samples from the game engine
// inBuffer should be filled with stereo Sint16 samples, little endian,
//    left-right left-right ....
// NOTE:  may not be called by the same thread that calls drawFrame,
//        depending on platform implementation
void getSoundSamples( Uint8 *inBuffer, int inLengthToFillInBytes );



// called BY game engine (implemented by supporting platform)

// true to start or resume playing
// false to pause
void setSoundPlaying( char inPlaying );






