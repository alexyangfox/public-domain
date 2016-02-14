#include "Panel.h"

#include "GridSpace.h"

#include "ScoreBundle.h"

#include "WebRequest.h"

#include "HighScorePanel.h"


class HighScoreLoadingPanel : public Panel {
        

    public:
        

        HighScoreLoadingPanel( int inW, int inH );
        

        ~HighScoreLoadingPanel();

        
        void setLoadingMessage();
        

        
        // bundle deleted after posting
        // posting starts when made visible
        void setScoreToPost( ScoreBundle *inBundle );


        // display panel is NOT managed as a sub panel of this panel
        HighScorePanel *getDisplayPanel() {
            return &mDisplayPanel;
            }
        

        // overrides these:
        void step();
        
        void setVisible( char inIsVisible );

        char pointerUp( int inX, int inY );
        

    protected:
        // override
        void drawBase();
        void closePressed();
        

        
    private:
        
        char *mMessage;
        char *mLastMessage;
        float mStringTransitionProgress;

        GridSpace mStatusLight;
        
        float mBlinkTime;
        
        char mTryingToPostScore;
        
        ScoreBundle *mScoreToPost;
        
        WebRequest *mWebRequest;

        
        char *mServerFinderURL;

        char *mServerURL;
        WebRequest *mServerURLFetchRequest;
        
        char mFailed;
        char mLightRed;
        

        void startConnectionTry();

        void setFailed();
        

        HighScorePanel mDisplayPanel;
        
        
        
    };


        
        
