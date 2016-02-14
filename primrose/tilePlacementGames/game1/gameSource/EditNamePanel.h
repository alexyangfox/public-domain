#ifndef EDIT_NAME_PANEL_INCLUDED
#define EDIT_NAME_PANEL_INCLUDED



#include "Panel.h"

#include "ScoreBundle.h"



class EditNamePanel : public Panel {
        

    public:
        

        EditNamePanel( int inW, int inH );
        

        ~EditNamePanel();
        

        // set score to post (with edited name) as soon as name
        // next edited.
        // after name edited, and SEND button pressed, 
        // postScore will be called from 
        // inScoreHandler (assumed to be a MenuPanel)
        // inScore destroyed by this class
        void setScoreToPost( ScoreBundle *inScore,
                             Panel *inScoreHandler );
        

        // over rides these:
        void setVisible( char inIsVisible );
        char pointerUp( int inX, int inY );
        

    protected:
        // override
        void drawBase();
        void closePressed();
        

        
    private:
        
        // pointer to global player name
        char *mName;

        // override instead of drawing global name if non-NULL
        char *mOverrideName;
        
        Button mSendButton;
        
        
        Button *mKeyButtons[28];
        

        ScoreBundle *mScoreToPost;
        Panel *mScoreHandler;
        
        
    };


#endif

        
        
