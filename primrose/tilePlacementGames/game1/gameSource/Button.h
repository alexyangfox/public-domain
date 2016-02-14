#ifndef BUTTON_INCLUDED
#define BUTTON_INCLUDED


#include "GridSpace.h"

class Button {
        
    public:
        
        // constructs w/ center location on screen and a string label
        // if label is 1 or 2 characters, drawn with big font
        // if 3 or 4 chars, drawn with little font
        Button( int inX, int inY, char *inString );
        
        ~Button();
        
        // resets string
        // smooth transition
        void setString( char *inString );

        // instant transtition
        void forceString( char *inString );


        char isInside( int inX, int inY );
        
        
        void setVisible( char inIsVisible );

        char isVisible() {
            return mVisible;
            }
        
        // forces to become completely visible instantly
        void forceVisible();
        void forceInvisible();
        

        // steps animations
        void step();
        

        // draws onto screen
        void draw();
        

        int mX, mY;

        char *mString;


    private:
        
        GridSpace mSpace;
        
        
        char mVisible;
        
        float mFadeProgress;
        
        char *mLastString;
        float mStringTransitionProgress;
        
    };


#endif
        
        
