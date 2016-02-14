#include "Panel.h"


#include "TutorialPanelC.h"



class TutorialPanelB : public Panel {
        

    public:
        

        TutorialPanelB( int inW, int inH );
        

        ~TutorialPanelB();
        
        

        // overrides these:
        char pointerUp( int inX, int inY );
        
        void setVisible( char inIsVisible );
        
        void step();
        

    protected:
        // override
        void drawBase();

        // receive close event
        void closePressed();
        
        
    private:
        
        void setStageZero();
        

        Button mNextButton;
        TutorialPanelC mNextPanel;
        

        
        GridSpace *mGridDemo[4][7];
        GridSpace *mAllDemoSpaces[28];
        

        // surrounded
        GridSpace *mInnerSpaces[4];
        // surrounding
        GridSpace *mOuterSpaces[7];
        // final to complete
        GridSpace *mKeySpace;
        
        int mInnerSpacesX[4];
        
            
        int mDemoStage;
        

        int mStepsBetweenStages;
        
        int mStepCount;
        
        
    };


        
        
