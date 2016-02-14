#ifndef SCORE_BUNDLE_INCLUDED
#define SCORE_BUNDLE_INCLUDED



class ScoreBundle {
        
    public:
        
        ScoreBundle( char *inName, 
                     unsigned int inScore, unsigned int inSeed,
                     char *inMoveHistory );
        
        // encoded as  name#score#seed#move_history
        // (#-delimited)
        ScoreBundle( char *inEncoded );
        
        
        ~ScoreBundle();
        

        ScoreBundle *copy();
        
        
        // name up to 8 chars long
        char mName[9];

        unsigned int mScore;
        

        unsigned int mSeed;
        

        char *mMoveHistory;

        int mNumMoves;

    };



#endif
        
