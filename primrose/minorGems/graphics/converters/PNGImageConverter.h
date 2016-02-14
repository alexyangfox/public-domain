/*
 * Modification History
 *
 * 2006-November-21   Jason Rohrer
 * Created.
 */
 
 
#ifndef PNG_IMAGE_CONVERTER_INCLUDED
#define PNG_IMAGE_CONVERTER_INCLUDED 


#include "BigEndianImageConverter.h"



/**
 * PNG implementation of the image conversion interface.
 *
 * Note that it only supports 24-bit PNG files
 * (and thus only 3-channel Images).
 *
 * TGA format information taken from:
 * http://www.cubic.org/source/archive/fileform/graphic/tga/targa.txt
 *
 * @author Jason Rohrer
 */
class PNGImageConverter : public BigEndianImageConverter {
	
	public:

        
        PNGImageConverter();
        
        
        
		// implement the ImageConverter interface
		virtual void formatImage( Image *inImage, 
			OutputStream *inStream );
			
		virtual Image *deformatImage( InputStream *inStream );		


    protected:

        /**
         * Writes a chunk to a stream.
         *
         * @param inChunkType the 4-char type of the chunk.
         * @param inData the data for the chunk.  Can be NULL if no data
         *   in chunk.  Destroyed by caller.
         * @param inNumBytes the length of inData, or 0 if inData is NULL.
         * @param inStream the stream to write the chunk to.  Destroyed by
         *   caller.
         */
        void writeChunk( char inChunkType[4], unsigned char *inData,
                         unsigned long inNumBytes, OutputStream *inStream );


        
        // precomputed CRCs for all 8-bit messages
        unsigned long mCRCTable[256];


        const static unsigned long mStartCRC = 0xffffffffL;


        
        /**
         * Updates a crc with new data.
         *
         * Note that starting state for a CRC (before it is updated with data)
         * must be mStartCRC.
         * After all data has been added to the CRC, the resulting value
         * must be inverted (crc ^ 0xffffffffL).
         *
         * @param inCRC the current crc value.
         * @param inData the data.  Destroyed by caller.
         * @param inLength the length of the data.
         *
         * @return the new CRC.
         */
        unsigned long updateCRC( unsigned long inCRC, unsigned char *inData,
                                 int inLength );

	};


		
#endif
