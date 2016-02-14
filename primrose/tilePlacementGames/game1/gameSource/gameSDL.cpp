// SDL platform that runs game.h engine


// let SDL override our main function with SDLMain
#include <SDL/SDL_main.h>


// must do this before SDL include to prevent WinMain linker errors on win32
int mainFunction( int inArgCount, char **inArgs );

int main( int inArgCount, char **inArgs ) {
    return mainFunction( inArgCount, inArgs );
    }


#include <SDL/SDL.h>



#include "game.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <GL/gl.h>


#include "minorGems/util/stringUtils.h"


// callbacks, originally for GLUT, called by our SDL main loop below
void callbackMotion( int inX, int inY );
// ignore passive motion
// void callbackPassiveMotion( int inX, int inY );
void callbackMouse( int inButton, int inState, int inX, int inY );
void callbackDisplay();
void callbackTimer( int inValue );

// keyboard to catch quit keys (don't pass to game)
void callbackKeyboard( unsigned char inKey, int inX, int inY );


void callbackReshape( int inW, int inH );




// maps SDL-specific special (non-ASCII) key-codes (SDLK) to minorGems key 
// codes (MG_KEY)
//int mapSDLSpecialKeyToMG( int inSDLKey );

// for ascii key
char mapSDLKeyToASCII( int inSDLKey );







void cleanUpAtExit() {
    printf( "Exiting...\n" );

    SDL_CloseAudio();
    
    freeFrameDrawer();    
    }



void audioCallback( void *inUserData, Uint8 *inStream, int inLengthToFill ) {
    getSoundSamples( inStream, inLengthToFill );
    }




int w = 320;
int h = 480;


// 25 fps
int frameMS = 40;
//int frameMS = 500;

// s and f keys to slow down and speed up for testing
char enableSlowdownKeys = false;
//char enableSlowdownKeys = true;



int mainFunction( int inNumArgs, char **inArgs ) {

    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE ) < 0 ) {
        printf( "Couldn't initialize SDL: %s\n", SDL_GetError() );
        return 0;
        }

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	
    int flags = SDL_OPENGL;
    

    //flags = flags | SDL_FULLSCREEN;
    
    // current color depth
    SDL_Surface *screen = SDL_SetVideoMode( w, h, 0, flags);


    if ( screen == NULL ) {
        printf( "Couldn't set %dx%d GL video mode: %s\n", 
                w, 
                h,
                SDL_GetError() );
        return 0;
        }
    

    SDL_WM_SetCaption( "Primrose", NULL );
    

    // turn off repeat
    SDL_EnableKeyRepeat( 0, 0 );



    
    #ifdef __mac__
        // make sure working directory is the same as the directory
        // that the app resides in
        // this is especially important on the mac platform, which
        // doesn't set a proper working directory for double-clicked
        // app bundles

        // arg 0 is the path to the app executable
        char *appDirectoryPath = stringDuplicate( inArgs[0] );
    
        char *appNamePointer = strstr( appDirectoryPath,
                                       "tileGame1.app" );

        if( appNamePointer != NULL ) {
            // terminate full app path to get parent directory
            appNamePointer[0] = '\0';
            
            chdir( appDirectoryPath );
            }
        
        delete [] appDirectoryPath;
    #endif



    



    SDL_AudioSpec audioFormat;

    /* Set 16-bit stereo audio at 22Khz */
    audioFormat.freq = gameSoundSampleRate;
    audioFormat.format = AUDIO_S16;
    audioFormat.channels = 2;
    audioFormat.samples = 512;        /* A good value for games */
    //audioFormat.samples = 1024;        
    //audioFormat.samples = 2048;        
    audioFormat.callback = audioCallback;
    audioFormat.userdata = NULL;

    /* Open the audio device and start playing sound! */
    if( SDL_OpenAudio( &audioFormat, NULL ) < 0 ) {
        printf( "Unable to open audio: %s\n", SDL_GetError() );
        }

    // set pause to 0 to start audio
    //SDL_PauseAudio(0);


    initFrameDrawer( w, h );
    
    // to free frame drawer, stop audio, etc
    atexit( cleanUpAtExit );


    Uint32 lastFrameDoneTime = SDL_GetTicks();

    Uint32 frameBatchStartTime = SDL_GetTicks();
    
    int frameBatchSize = 100;
    int frameBatchCount = 0;
    
    

    
    // main loop
    while( true ) {
        
        callbackDisplay();
        

        // handle all pending events
        SDL_Event event;
        
        while( SDL_PollEvent( &event ) ) {
            
            switch( event.type ) {
                case SDL_QUIT:
                    exit( 0 );
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP: {
                    int mouseX, mouseY;
                    SDL_GetMouseState( &mouseX, &mouseY );
                    
                    
                    // check if special key
                    //int mgKey = mapSDLSpecialKeyToMG( event.key.keysym.sym );
                    // ignore special keys for now
                    int mgKey = 0;
                    
                    
                    if( mgKey != 0 ) {
                        if( event.type == SDL_KEYDOWN ) {
                            //callbackSpecialKeyboard( mgKey, mouseX, mouseY );
                            }
                        else {
                            //callbackSpecialKeyboardUp( mgKey, 
                            // mouseX, mouseY );
                            }
                        }
                    else {
                        char asciiKey = 
                            mapSDLKeyToASCII( event.key.keysym.sym );
                      
                        if( asciiKey != 0 ) {
                            // shift and caps cancel each other
                            if( ( ( event.key.keysym.mod & KMOD_SHIFT )
                                  &&
                                  !( event.key.keysym.mod & KMOD_CAPS ) )
                                ||
                                ( !( event.key.keysym.mod & KMOD_SHIFT )
                                  &&
                                  ( event.key.keysym.mod & KMOD_CAPS ) ) ) {
                                
                                asciiKey = toupper( asciiKey );
                                }
                        
                            if( event.type == SDL_KEYDOWN ) {
                                callbackKeyboard( asciiKey, mouseX, mouseY );
                                }
                            else {
                                //callbackKeyboardUp( asciiKey, 
                                // mouseX, mouseY );
                                }
                            }
                        }
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if( event.motion.state & SDL_BUTTON( 1 )
                        || 
                        event.motion.state & SDL_BUTTON( 2 )
                        ||
                        event.motion.state & SDL_BUTTON( 3 ) ) {
                        
                        callbackMotion( event.motion.x, event.motion.y );
                        }
                    else {
                        //callbackPassiveMotion( event.motion.x, 
                        // event.motion.y );
                        }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    callbackMouse( event.button.button,
                                   event.button.state,
                                   event.button.x, 
                                   event.button.y );
                    break;
                }
            }

        Uint32 thisFrameDoneTime = SDL_GetTicks();
        
        int extraMS = frameMS - ( thisFrameDoneTime - lastFrameDoneTime );
        
        if( extraMS > 0 ) {
            SDL_Delay( extraMS );
            }

        // set to ideal frame done time (even though SDL_Delay might
        // be slightly longer or shorter

        // this way, we don't fall gradually out of time
        lastFrameDoneTime = thisFrameDoneTime + extraMS;


        frameBatchCount ++;
        
        if( frameBatchCount >= frameBatchSize ) {
            Uint32 doneTime = SDL_GetTicks();
            Uint32 totalTime =  doneTime - frameBatchStartTime;
            
            float fps = (float)frameBatchCount / (totalTime / 1000.0f);
            
            if( false ) {
                printf( "FPS = %f\n", fps );
                }
            


            // new batch
            frameBatchStartTime = doneTime;
            frameBatchCount = 0;
            }
        
        }


    return 0;    
    }



void setSoundPlaying( char inPlaying ) {
    printf( "Setting audio playing to %d\n", inPlaying );
    
    SDL_PauseAudio( !inPlaying );
    }




void callbackDisplay() {
		
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho( 0, 320, 480, 0, -1.0f, 1.0f);
    
    
    glMatrixMode(GL_MODELVIEW);

    glDisable( GL_TEXTURE_2D );
	glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
    
    drawFrame();
    
	
    SDL_GL_SwapBuffers();
	}




void callbackMotion( int inX, int inY ) {
    pointerMove( inX, inY );
	}
		
	
	
void callbackMouse( int inButton, int inState, int inX, int inY ) {
	if( inState == SDL_PRESSED ) {
        pointerDown( inX, inY );
        }
    else if( inState == SDL_RELEASED ) {
        pointerUp( inX, inY );
        }
    else {
        printf( "Error:  Unknown mouse state received from SDL\n" );
        }	
    }


void callbackKeyboard( unsigned char inKey, int inX, int inY ) {
    
    // q or escape
    if( inKey == 'q' || inKey == 'Q' || inKey == 27 ) {
        exit( 0 );
        }
    

    if( enableSlowdownKeys ) {
        
        if( inKey == 's' || inKey == 'S' ) {
            // slow
            frameMS = 500;
            }
        if( inKey == 'f' || inKey == 'F' ) {
            // normal
            frameMS = 40;
            }
        }
    
    
	}


/*
int mapSDLSpecialKeyToMG( int inSDLKey ) {
    switch( inSDLKey ) {
        case SDLK_F1: return MG_KEY_F1;
        case SDLK_F2: return MG_KEY_F2;
        case SDLK_F3: return MG_KEY_F3;
        case SDLK_F4: return MG_KEY_F4;
        case SDLK_F5: return MG_KEY_F5;
        case SDLK_F6: return MG_KEY_F6;
        case SDLK_F7: return MG_KEY_F7;
        case SDLK_F8: return MG_KEY_F8;
        case SDLK_F9: return MG_KEY_F9;
        case SDLK_F10: return MG_KEY_F10;
        case SDLK_F11: return MG_KEY_F11;
        case SDLK_F12: return MG_KEY_F12;
        case SDLK_LEFT: return MG_KEY_LEFT;
        case SDLK_UP: return MG_KEY_UP;
        case SDLK_RIGHT: return MG_KEY_RIGHT;
        case SDLK_DOWN: return MG_KEY_DOWN;
        case SDLK_PAGEUP: return MG_KEY_PAGE_UP;
        case SDLK_PAGEDOWN: return MG_KEY_PAGE_DOWN;
        case SDLK_HOME: return MG_KEY_HOME;
        case SDLK_END: return MG_KEY_END;
        case SDLK_INSERT: return MG_KEY_INSERT;
        default: return 0;
        }
    }
*/

char mapSDLKeyToASCII( int inSDLKey ) {
    switch( inSDLKey ) {
        case SDLK_UNKNOWN: return 0;
        case SDLK_BACKSPACE: return 8;
        case SDLK_TAB: return 9;
        case SDLK_CLEAR: return 12;
        case SDLK_RETURN: return 13;
        case SDLK_PAUSE: return 19;
        case SDLK_ESCAPE: return 27;
        case SDLK_SPACE: return ' ';
        case SDLK_EXCLAIM: return '!';
        case SDLK_QUOTEDBL: return '"';
        case SDLK_HASH: return '#';
        case SDLK_DOLLAR: return '$';
        case SDLK_AMPERSAND: return '&';
        case SDLK_QUOTE: return '\'';
        case SDLK_LEFTPAREN: return '(';
        case SDLK_RIGHTPAREN: return ')';
        case SDLK_ASTERISK: return '*';
        case SDLK_PLUS: return '+';
        case SDLK_COMMA: return ',';
        case SDLK_MINUS: return '-';
        case SDLK_PERIOD: return '.';
        case SDLK_SLASH: return '/';
        case SDLK_0: return '0';
        case SDLK_1: return '1';
        case SDLK_2: return '2';
        case SDLK_3: return '3';
        case SDLK_4: return '4';
        case SDLK_5: return '5';
        case SDLK_6: return '6';
        case SDLK_7: return '7';
        case SDLK_8: return '8';
        case SDLK_9: return '9';
        case SDLK_COLON: return ':';
        case SDLK_SEMICOLON: return ';';
        case SDLK_LESS: return '<';
        case SDLK_EQUALS: return '=';
        case SDLK_GREATER: return '>';
        case SDLK_QUESTION: return '?';
        case SDLK_AT: return '@';
        case SDLK_LEFTBRACKET: return '[';
        case SDLK_BACKSLASH: return '\\';
        case SDLK_RIGHTBRACKET: return ']';
        case SDLK_CARET: return '^';
        case SDLK_UNDERSCORE: return '_';
        case SDLK_BACKQUOTE: return '`';
        case SDLK_a: return 'a';
        case SDLK_b: return 'b';
        case SDLK_c: return 'c';
        case SDLK_d: return 'd';
        case SDLK_e: return 'e';
        case SDLK_f: return 'f';
        case SDLK_g: return 'g';
        case SDLK_h: return 'h';
        case SDLK_i: return 'i';
        case SDLK_j: return 'j';
        case SDLK_k: return 'k';
        case SDLK_l: return 'l';
        case SDLK_m: return 'm';
        case SDLK_n: return 'n';
        case SDLK_o: return 'o';
        case SDLK_p: return 'p';
        case SDLK_q: return 'q';
        case SDLK_r: return 'r';
        case SDLK_s: return 's';
        case SDLK_t: return 't';
        case SDLK_u: return 'u';
        case SDLK_v: return 'v';
        case SDLK_w: return 'w';
        case SDLK_x: return 'x';
        case SDLK_y: return 'y';
        case SDLK_z: return 'z';
        case SDLK_DELETE: return 127;
        default: return 0;
        }
    }

    


