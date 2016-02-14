#include "Panel.h"

#include "ColorPool.h"


class TutorialPanelC : public Panel {
        

    public:
        

        TutorialPanelC( int inW, int inH );
        

        ~TutorialPanelC();
        
        

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
        
        ColorPool mColorPool;
        
        
        GridSpace *mGridDemo[4][7];
        GridSpace *mAllDemoSpaces[28];
        

        // surrounded
        GridSpace *mInnerSpacesA[1];
        // surrounding
        GridSpace *mOuterSpacesA[3];

        // surrounded
        GridSpace *mInnerSpacesB[4];
        // surrounding
        GridSpace *mOuterSpacesB[6];

        // final to complete
        GridSpace *mKeySpace;
        
        int mInnerSpacesAX[1];
        
        int mInnerSpacesBX[4];
            
        int mDemoStage;
        

        int mStepsBetweenStages;
        
        int mStepCount;
        
        
    };


        
        
