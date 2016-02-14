#ifndef PANEL_INCLUDED
#define PANEL_INCLUDED


#include "Button.h"


#include "minorGems/util/SimpleVector.h"



// abstract base class
// panel with a close button at the top
class Panel {
        

    public:
        

        Panel( int inW, int inH);
        

        virtual ~Panel();
        

                
        virtual void setVisible( char inIsVisible );

        virtual char isVisible() {
            return mVisible;
            }
        
        // forces to become completely visible/invisible instantly
        virtual void forceVisible();
        virtual void forceInvisible();
        

        virtual char isFullyVisible() {
            return mVisible && (mFadeProgress == 1);
            }
        

        char isSubPanelVisible();



        // tells panel that pointer released inside it
        // returns true if release consumed (triggered a button somewhere)
        virtual char pointerUp( int inX, int inY );
        


        // steps animations
        virtual void step();
        

        // draws onto screen
        // don't override this
        void draw();


    protected:
        
        void addSubPanel( Panel *inPanel );
        void addButton( Button *inButton );
        
        
        char isSubPanelFullyVisible();
        

        // subclasses override these
        virtual void drawBase();
        virtual void closePressed() = 0;
                
        
        int mW, mH;

        float mFadeProgress;
        
        char mVisible;


    private:
        void drawSubPanels();

        Button mCloseButton;
        
        SimpleVector<Button *> mButtons;

        SimpleVector<Panel *> mSubPanels;
        
        
        
    };


#endif

        
        
