#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "game.h"


#include <math.h>

#include <stdio.h>


class SoundSample {
    public:
        SoundSample() 
                : mNumSamples( -1 ), 
                  mLeftChannel( NULL ), mRightChannel( NULL ) {
            }
        
        ~SoundSample() {
            if( mLeftChannel != NULL ) {
                delete [] mLeftChannel;
                }
            if( mRightChannel != NULL ) {
                delete [] mRightChannel;
                }
            }
        
        // returns true on success
        char readFromSettingsFile( char *inSettingName );
        
        void writeToSettingsFile( char *inSettingName );
        

        int mNumSamples;
        
        // samples in [-1, +1]
        float *mLeftChannel;
        float *mRightChannel;
    };




// 7, one for each placement
// +
// 7 for chain steps
#define numSamplesInBank 14


SoundSample sampleBank[ numSamplesInBank ];



class ActiveSound {
    public:
        
        ActiveSound( int inBankIndex, 
                     float inLeftLoudness, float inRightLoudness ) 
                : mBankIndex( inBankIndex ), mSamplesPlayed( 0 ),
                  mLeftLoudness( inLeftLoudness ),
                  mRightLoudness( inRightLoudness ) {
            }
        

        int mBankIndex;        
        int mSamplesPlayed;
        float mLeftLoudness, mRightLoudness;
    };


SimpleVector<ActiveSound*> activeSounds;





double twelthRootOfTwo = pow( 2, 1.0/12 );

// for major scale
// W, W, H, W, W, W, H
//int halfstepMap[ 7 ] = { 0, 2, 4, 5, 7, 9, 11 };

// minor scale
// W,H,W,W,H,W,W
//int halfstepMap[ 7 ] = { 0, 2, 3, 5, 7, 8, 10 };


// our haunting chord
//int halfstepMap[ 7 ] = { 0, 3, 7, 10, 12, 17, 22 };


// packed into one octave (and not in order!
//int halfstepMap[ 7 ] = { 3, 6, 11, 8, 4, 10, 1 };

// new minor scale, not in order
int placementStepMap[ 7 ] = { 0, 3, 7, 10, 8, 5, 2 };


// major scale
int clearingStepMap[ 7 ] = { 0, 2, 4, 5, 7, 9, 11 };




// some primitive wave functions

// 40 produces fine results (almost perfect square wave)
//int nLimit = 40;
// 20 sounds smoother, less buzzy

// note that this is just a maximum value
// actual value to pass to wave functions depends on nyquist limit and 
// desired frequency of wave
int nLimitMax= 20;
//int nLimit = 10;
//int nLimit = 80;
//int nLimit = 5;

// precomputed 1/n
double nCoefficients[20];


// square wave with period of 2pi
inline double squareWave( double inT, int inNLimit ) {
    double sum = 0;
    
    for( int n=1; n<inNLimit; n+=2 ) {
        sum +=  nCoefficients[n] * sin( n * inT );
        }
    return sum;
    }



// sawtoot wave with period of 2pi
double sawWave( double inT, int inNLimit ) {
    double sum = 0;
    
    for( int n=1; n<inNLimit; n++ ) {
        sum += 1.0/n * sin( n * inT );
        }
    return sum;
    }



void fillTone( SoundSample *inSample, float inFrequency, int inNumSamples,
               int inNumSamplesToSkip = 0, int inTotalSamples = -1 ) {

    SoundSample *s = inSample;
    
    if( inTotalSamples == -1 ) {
        inTotalSamples = inNumSamples + inNumSamplesToSkip;
        }
    

    if( s->mNumSamples == -1 ) {
        
        s->mNumSamples = inTotalSamples;    
        
        s->mLeftChannel = new float[ inTotalSamples ];
        s->mRightChannel = new float[ inTotalSamples ];
        }

    float *l = s->mLeftChannel;
    float *r = s->mRightChannel;
        
    
    // nLimit for wave function (limit on number of frequency components)
    // based on Nyquist
    int nyquist = gameSoundSampleRate / 2;
    int nLimit = (int)( nyquist / inFrequency );
    
    //printf( "nLimit = %d\n", nLimit );
    
    if( nLimit > nLimitMax ) {
        nLimit = nLimitMax;
        
        //printf( "capping nLimit at %d\n", nLimit );
        }
    


    float sinFactor = (1.0f / gameSoundSampleRate) * inFrequency * 2 * M_PI;
    
    float envSinFactor = 1.0f / inNumSamples * M_PI;
    
    int limit = inNumSamplesToSkip + inNumSamples;
    
    for( int i=inNumSamplesToSkip; i!=limit; i++ ) {
        float value = squareWave( i * sinFactor, nLimit );
        
        // apply another sin as an envelope
        l[i] = value * sin( i * envSinFactor );
        }

    // channels identical
    memcpy( & r[inNumSamplesToSkip], & l[inNumSamplesToSkip ],
            inNumSamples * sizeof( float ) );
    
    }






void initSound() {
    // precomput nCoefficients for square wave
    
    for( int n=1; n<nLimitMax; n++ ) {
        nCoefficients[n] = 1.0 / n;
        }
    


    float baseFreq = 60;


    int bankIndex = 0;
    
    int i;
    
    int numGenerated = 0;
    int numCached = 0;
    

    // for each color
    for( i=0; i<7; i++ ) {
        
        // placement sounds
        float freq = baseFreq * pow( twelthRootOfTwo, placementStepMap[i] );
        
        SoundSample *s = &( sampleBank[bankIndex] );
        
        // check if cached on disk
        char *cacheSettingName = autoSprintf( "cachedSoundSample_%d",
                                              bankIndex );

        if( ! s->readFromSettingsFile( cacheSettingName ) ) {
            // not cached, generate
            //printf( "Generating sound sample %d\n", bankIndex );
            

            int numSamples = (int)( 0.2 * gameSoundSampleRate );
            
            fillTone( s, freq, numSamples );
            
            // cache out to disk
            s->writeToSettingsFile( cacheSettingName );

            numGenerated ++;
            }
        else {
            //printf( "Found sound sample %d cached on disk\n", bankIndex );
            numCached ++;
            }
        
        delete [] cacheSettingName;
        

        bankIndex ++;
        }
    
    // now clearing sounds
    // shepard tones

    
    for( i=0; i<7; i++ ) {
        SoundSample *s = &( sampleBank[ bankIndex ] );

        // check if cached on disk
        char *cacheSettingName = autoSprintf( "cachedSoundSample_%d",
                                              bankIndex );

        if( ! s->readFromSettingsFile( cacheSettingName ) ) {
            // not cached, generate
            //printf( "Generating sound sample %d\n", bankIndex );

            float freqA = baseFreq * pow( twelthRootOfTwo, 
                                          clearingStepMap[i] );
            
            // second, 7 steps up
            float freqB = baseFreq * pow( twelthRootOfTwo, 
                                          clearingStepMap[i] + 7 );

            #define numShepParts 4
            int t;
        
            SoundSample shepardParts[numShepParts];
            int numSamples = (int)( 0.4 * gameSoundSampleRate );

            for( t=0; t<numShepParts; t++ ) {
                // first note in sequence
                fillTone( &shepardParts[t], freqA, 
                          numSamples / 2, 0, numSamples );
            
                // second
                fillTone( &shepardParts[t], freqB, 
                          numSamples/2, numSamples/2, numSamples );
            
                // raise for next shep part
                freqA *= 2;
                freqB *= 2;
                }
        

            float partWeights[4];
        
            // 0.0 .. 0.5
            partWeights[0] = i / 12.0f;

            // 0.5 .. 1.0
            partWeights[1] = 0.5 + partWeights[0];
        
            // 1.0 .. 0.5
            partWeights[2] = 1 - partWeights[0];
        
            // 0.5 .. 0.0
            partWeights[3] = 0.5 - partWeights[0];
        

            // vol sum at any point = 2
        
            // weight so sum = 1
            for( int t=0; t<numShepParts; t++ ) {
                partWeights[t] *= 0.5;
                }
        

            s->mNumSamples = numSamples;

            s->mLeftChannel = new float[ numSamples ];
            s->mRightChannel = new float[ numSamples ];
        

            float *l = s->mLeftChannel;
            float *r = s->mRightChannel;
        
            // accumulate in left
            // copy to right at end
            for( int j=0; j<numSamples; j++ ) {
                l[j] = 0;
                }
        

            for( t=0; t<numShepParts; t++ ) {

                float weight = partWeights[t];
            
                // channels identical
                // use only one
                float *partChannel = shepardParts[t].mLeftChannel;
            

                for( int j=0; j<numSamples; j++ ) {
                    l[j] += weight * partChannel[j];
                    }
                }

            // copy to right
            memcpy( r, l, numSamples * sizeof( float ) );


            // save out to disk
            s->writeToSettingsFile( cacheSettingName );

            numGenerated++;
            }
        else {
            //printf( "Found sound sample %d cached on disk\n", bankIndex );
            numCached++;
            }

        
        
        delete [] cacheSettingName;
        
        bankIndex ++;
        }

    printf( "Generated %d sound samples fresh, found %d cached on disk\n", 
            numGenerated, numCached );
    }



void freeSound() {
    
    for( int i=0; i<activeSounds.size(); i++ ) {
        delete *( activeSounds.getElement( i ) );
        }
    
    }



void playPlacementSound( int inColor, 
                         float inLeftLoudness, float inRightLoudness ) {

    //printf( "Playing sound\n" );
    
    // louder than that of clearing group size of 1
    // (clearing sound ends up sounding louder)
    float loudness = 0.5;
    
    activeSounds.push_back( new ActiveSound( inColor, 
                                             inLeftLoudness * loudness,
                                             inRightLoudness * loudness ) );
    }



void playClearingSound( int inColor, int inGroupSize, int inChainLength,
                        float inLeftLoudness, float inRightLoudness ) {
    // asymptotically approaches 0.5 as inGroupSize grows.
    // very close to 0.5 when inGroupSize is 49
    float loudness =
        0.75 * (1 - pow(10, (-inGroupSize/20.0f) ) ) + 0.25;


    inChainLength -= 1;
    
    inChainLength = inChainLength % 7;
    
    int index = 7 + inChainLength;
    
    //printf( "playing sound from bank %d with loudness %f, l, r = %f, %f\n", 
    //        index, loudness, inLeftLoudness, inRightLoudness );
    
    activeSounds.push_back( new ActiveSound( index, 
                                             loudness * inLeftLoudness, 
                                             loudness * inRightLoudness ) );
    }
  


int numTestSoundsPlayed = 0;
int stepsBetweenTestPlays = 0;


// implements getSoundSamples from game.h
void getSoundSamples( Uint8 *inBuffer, int inLengthToFillInBytes ) {
    //printf( "Audio callback\n" );
    
    // 2 16-bit samples per frame
    int numFrames = inLengthToFillInBytes / 4;
    

    float *leftMix = new float[ numFrames ];
    float *rightMix = new float[ numFrames ];
    
    int f;

    for( f=0; f!=numFrames; f++ ) {
        leftMix[f] = 0;
        rightMix[f] = 0;
        }

      
    int i = 0;
    
    // we may be removing sounds from the buffer as we use them up
    // i is adjusted inside the while loop
    while( i<activeSounds.size() ) {
        
        ActiveSound *a = *( activeSounds.getElement( i ) );

        int samplesToSkip = a->mSamplesPlayed;

        
        SoundSample *s = &( sampleBank[ a->mBankIndex ] );
        

        int mixLength = s->mNumSamples - samplesToSkip;

        if( mixLength > numFrames ) {
            mixLength = numFrames;
            }

        
        float leftLoudness = a->mLeftLoudness;
        float rightLoudness = a->mRightLoudness;

        float *sampleLeft = s->mLeftChannel;
        float *sampleRight = s->mRightChannel;

        for( int j=0; j != mixLength; j++ ) {
            leftMix[j] += leftLoudness * sampleLeft[j + samplesToSkip];
            rightMix[j] += rightLoudness * sampleRight[j + samplesToSkip];
            }
        
        a->mSamplesPlayed += mixLength;
        
        if( a->mSamplesPlayed >= s->mNumSamples ) {
            // sound done playing
        
            delete a;
            activeSounds.deleteElement( i );
        
            // don't increment i
            }
        else {
            // next active sound
            i++;
            }
        }

    /*
    if(  activeSounds.size() == 0 && numTestSoundsPlayed < numSamplesInBank ) {
        if( stepsBetweenTestPlays > 10 ) {
            stepsBetweenTestPlays = 0;
            
            // play next test sound
            printf( "Test play of bank sound %d\n", numTestSoundsPlayed );
            
         
            float loudness = 0.5;
            if( numTestSoundsPlayed >=7 ) {
                loudness = 0.25;
                }
            
            activeSounds.push_back( 
                new ActiveSound( numTestSoundsPlayed, loudness ) );
            
            //playClearingSound( 0, 1, numTestSoundsPlayed + 1 );
            
            numTestSoundsPlayed ++;
            }
        else {
            stepsBetweenTestPlays++;
            }
        
        }
    */
        

    #define Sint16Max 32767

    // now copy samples into Uint8 buffer (converting them to Sint16s)
    int streamPosition = 0;
    for( f=0; f != numFrames; f++ ) {
        Sint16 intSampleL = (Sint16)( leftMix[f] * Sint16Max );
        Sint16 intSampleR = (Sint16)( rightMix[f] * Sint16Max );
        //printf( "Outputting samples %d, %d\n", intSampleL, intSampleR );

        inBuffer[ streamPosition ] = (Uint8)( intSampleL & 0xFF );
        inBuffer[ streamPosition + 1 ] = (Uint8)( ( intSampleL >> 8 ) & 0xFF );
        
        inBuffer[ streamPosition + 2 ] = (Uint8)( intSampleR & 0xFF );
        inBuffer[ streamPosition + 3 ] = (Uint8)( ( intSampleR >> 8 ) & 0xFF );
        
        streamPosition += 4;
        }
    
    delete [] leftMix;
    delete [] rightMix;
    
    }



char SoundSample::readFromSettingsFile( char *inSettingName ) {
    char *lengthSettingName = autoSprintf( "%s_numSamples", inSettingName );
    
    char found;
    int numSamples = SettingsManager::getIntSetting( lengthSettingName, 
                                                     &found );
    delete [] lengthSettingName;
    
    if( !found ) {
        return false;
        }
    
    FILE *file = SettingsManager::getSettingsFile( inSettingName, "rb" );
    
    if( file == NULL ) {
        return false;
        }
    

    int bytesPerChannel = numSamples * sizeof( float );
    
    // read channels
    float *channels[2];

    int c;
    
    char error = false;
    
    for( c=0; c<2; c++ ) {
        
        channels[c] = new float[ numSamples ];
        
        int numRead = 
            fread( (char *)( channels[c] ), 1, bytesPerChannel, file );

        if( numRead != bytesPerChannel ) {
            printf( "Error reading from settingsFile %s for channel %d\n",
                    inSettingName, c );
            error = true;
            }
        }

    fclose( file );
    
    if( ! error ) {
        
        mLeftChannel = channels[0];
        mRightChannel = channels[1];
        mNumSamples = numSamples;
        return true;
        }
    else {
        for( c=0; c<2; c++ ) {
            delete [] channels[c];
            }
        return false;
        }
    
    }


        
void SoundSample::writeToSettingsFile( char *inSettingName ) {
    char *lengthSettingName = autoSprintf( "%s_numSamples", inSettingName );
    
    SettingsManager::setSetting( lengthSettingName, mNumSamples );
    
    delete [] lengthSettingName;


    FILE *file = SettingsManager::getSettingsFile( inSettingName, "wb" );
    
    if( file == NULL ) {
        printf( "Error opening settings file %s\n", inSettingName );
        return;
        }

    float *channels[2];
    channels[0] = mLeftChannel;
    channels[1] = mRightChannel;
    
    int bytesPerChannel = mNumSamples * sizeof( float );


    for( int c=0; c<2; c++ ) {
        
        int numWritten = 
            fwrite( (char *)( channels[c] ), 1, bytesPerChannel, file );

        if( numWritten != bytesPerChannel ) {
            printf( "Error writing to settingsFile %s for channel %d\n",
                    inSettingName, c );
            }
        }

    fclose( file );
    }





