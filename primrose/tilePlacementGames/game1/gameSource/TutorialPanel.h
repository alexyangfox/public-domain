#include "Panel.h"

#include "NextPieceDisplay.h"

#include "TutorialPanelB.h"



class TutorialPanel : public Panel {
        

    public:
        

        TutorialPanel( int inW, int inH );
        

        ~TutorialPanel();
        
        

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
        TutorialPanelB mNextPanel;
        

        NextPieceDisplay mNextPieceDemo;
        
        GridSpace *mGridDemo[4][7];
        GridSpace *mAllDemoSpaces[28];


        int mDemoStage;
        

        int mStepsBetweenStages;
        
        int mStepCount;
        
        
    };


        
        
