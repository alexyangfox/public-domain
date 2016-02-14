/*
 * Modification History
 *
 * 2006-November-21   Jason Rohrer
 * Created.
 */


#include "PNGImageConverter.h"

#include "minorGems/util/SimpleVector.h"

//#include <zlib.h>

PNGImageConverter::PNGImageConverter() {

    // set up the CRC table
    
    // code taken from the PNG spec:
    // http://www.w3.org/TR/2003/REC-PNG-20031110/#D-CRCAppendix
    unsigned long c;
    int n, k;
   
    for( n=0; n<256; n++ ) {
        c = (unsigned long)n;
        for( k=0; k<8; k++ ) {
            if( c & 1 ) {
                c = 0xedb88320L ^ (c >> 1);
                }
            else {
                c = c >> 1;
                }
            }
        mCRCTable[n] = c;
        }
    }



unsigned long PNGImageConverter::updateCRC(
    unsigned long inCRC, unsigned char *inData, int inLength ) {

    // code taken from the PNG spec:
    // http://www.w3.org/TR/2003/REC-PNG-20031110/#D-CRCAppendix
    
    unsigned long c = inCRC;
    int n;
   
    for( n=0; n<inLength; n++ ) {
        c = mCRCTable[ (c ^ inData[n]) & 0xff ]
            ^
            (c >> 8);
        }
    return c;    
    }



#define ADLER_BASE 65521 /* largest prime smaller than 65536 */

/**
 * Updates an adler32 checksum.
 * code found here  http://www.ietf.org/rfc/rfc1950.txt
 *
 * New adlers should start with inAdler set to 1.
 *
 * @param inAdler the current state of the checksum.
 * @param inData the data to add.  Destroyed by caller.
 * @param inLength the length of the data in bytes.
 *
 * @return the new checksum.
 */
unsigned long updateAdler32( unsigned long inAdler,
                             unsigned char *inData, int inLength ) {
    unsigned long s1 = inAdler & 0xffff;
    unsigned long s2 = (inAdler >> 16) & 0xffff;
    int n;

    for (n = 0; n < inLength; n++) {
        s1 = (s1 + inData[n]) % ADLER_BASE;
        s2 = (s2 + s1)     % ADLER_BASE;
        }
    return (s2 << 16) + s1;
    }



void PNGImageConverter::writeChunk(
    char inChunkType[4], unsigned char *inData,
    unsigned long inNumBytes, OutputStream *inStream ) {

    // chunk layout:
    
    // 4-byte length
    // 4-char type
    // data
    // 4-byte CRC (applied to type and data parts)


    // write the length
    writeBigEndianLong( inNumBytes, inStream );

    inStream->write( (unsigned char *)inChunkType, 4 );

    // start the crc
    unsigned long crc = updateCRC( mStartCRC,
                                   (unsigned char *)inChunkType, 4 );

    if( inData != NULL ) {
        // chunk has data
        
        inStream->write( inData, inNumBytes );

        crc = updateCRC( crc, inData, inNumBytes );
        }

    // final step:  invert the CRC
    crc = crc ^ 0xffffffffL;
    
    // now write the CRC
    writeBigEndianLong( crc, inStream );
    }



void PNGImageConverter::formatImage( Image *inImage, 
	OutputStream *inStream ) {

	int numChannels = inImage->getNumChannels();
	
	// make sure the image is in the right format
	if( numChannels != 3 &&
		numChannels != 4 ) {
		printf( "Only 3- and 4-channel images can be converted to " );
		printf( "the PNG format.\n" );
		return;
		}

	long width = inImage->getWidth();
	long height = inImage->getHeight();
	
    // same for all PNG images
    // used to check for basic transmission errors, such as line-end flipping
    unsigned char pngSignature[8] = { 0x89, 0x50, 0x4E, 0x47,
                                      0x0D, 0x0A, 0x1A, 0x0A };

    inStream->write( pngSignature, 8 );
	

    // data for IHDR chunk
    unsigned char headerData[13];

    // width
    headerData[0] = (width >> 24) & 0xff;
    headerData[1] = (width >> 16) & 0xff;
    headerData[2] = (width >> 8) & 0xff;
    headerData[3] = width & 0xff;

    // height
    headerData[4] = (height >> 24) & 0xff;
    headerData[5] = (height >> 16) & 0xff;
    headerData[6] = (height >> 8) & 0xff;
    headerData[7] = height & 0xff;

    // bit depth
    headerData[8] = 8;

    // color type
    // 2 = truecolor (RGB)
    headerData[9] = 2;

    // compression method
    // method 0  (deflate)
    headerData[10] = 0;

    // filter method
    // method 0 supports 5 filter types
    headerData[11] = 0;

    // no interlace
    headerData[12] = 0;

    writeChunk( "IHDR", headerData, 13, inStream );


    int numRawBytes = width * height * 3;
    // extra byte per scanline for filter type
    numRawBytes += height;

    unsigned char *rawScanlineBytes = new unsigned char[ numRawBytes ];

    // ignore alpha channel
    double *channels[3];
    int i;
    for( i=0; i<3; i++ ) {
        channels[i] = inImage->getChannel( i );
        }

    int pixelNumber = 0;
    for( int y=0; y<height; y++ ) {
        // each scanline starts with filter type byte
        // using filter type 0 (no filtering)

        int index = y * ( width * 3 + 1 );

        // filter type
        rawScanlineBytes[ index ] = 0;

        index++;

        for( int x=0; x<width; x++ ) {

            for( i=0; i<3; i++ ) {
                rawScanlineBytes[ index ] =
                    (unsigned char)( 255 * channels[i][pixelNumber] );

                index ++;
                }

            pixelNumber++;
            }        
        }


    // zlib block contains
    // zlib compression method  (1 byte)
    // flags  (1 byte)
    // compressed data blocks
    // check value (4 bytes) (not CRC)

    SimpleVector<unsigned char> zlibBlock;

    // compression method 8 (deflate)
    // with a LZ77 window size parameter of w=7
    // LZ77 window size is then 2^( w + 8 ), or in this case 32768
    zlibBlock.push_back( 0x78 );

    // flags
    // compression level 0  (2 bytes = 00b)
    // no preset dictionary (1 byte = 0b)
    // check bits for compression method (5 bits)
    //   Should be such that if the 8-bit compression method, followed
    //   by the 8-bit flags field, is viewed as a 16-bit number,
    //   it is an even multiple of 31
    //   For our settings, check bits of 00001 works
    //zlibBlock.push_back( 0x01 );
    // hack:  mimic zlib here
    zlibBlock.push_back( 0xda );

    
    // now ready for compressed data blocks

    int rawDataIndex = 0;

    // length field is 16 bits
    int maxUncompressedBlockLength = 65535;
    
    while( rawDataIndex < numRawBytes ) {

        // push another deflate block

        // first bit BFINAL
        //   only 1 for final block
        // next two bits BTYPE
        //   BTYPE=00 is an uncompressed block
        // remaining 5 bits ignored
        // Thus, we put 0x00 for uncompressed blocks that are not final
        // and 0x80 for final uncompressed block

        
        int bytesLeft = numRawBytes - rawDataIndex;

        int bytesInBlock;
        
        if( bytesLeft <= maxUncompressedBlockLength ) {
            // final

            // hack:  when comparing output with zlib, noticed that it doesn't
            // set 0x80 for the final block, instead it uses 0x01
            // For some reason, this was making the PNG unreadable
            // zlibBlock.push_back( 0x80 );
            zlibBlock.push_back( 0x01 );

            bytesInBlock = bytesLeft;
            }
        else {
            // not final
            zlibBlock.push_back( 0x00 );
            
            bytesInBlock = maxUncompressedBlockLength;
            }

        // length in least-significant-byte-first order
        unsigned char firstLengthByte = bytesInBlock & 0xff;
        unsigned char secondLengthByte = (bytesInBlock >> 8) & 0xff;
        zlibBlock.push_back( firstLengthByte );
        zlibBlock.push_back( secondLengthByte );

        // those same length bytes inverted
        // (called "one's compliment" in the spec
        zlibBlock.push_back( firstLengthByte ^ 0xff);
        zlibBlock.push_back( secondLengthByte ^ 0xff );

        // now the uncompressed data

        for( int b=0; b< bytesInBlock; b++ ) {
            zlibBlock.push_back( rawScanlineBytes[ rawDataIndex ] );

            rawDataIndex++;
            }
        }
    

    // finally, adler32 of original data
    unsigned long adler = updateAdler32( 1L, rawScanlineBytes, numRawBytes );
    
    zlibBlock.push_back( (adler >> 24) & 0xff );
    zlibBlock.push_back( (adler >> 16) & 0xff );
    zlibBlock.push_back( (adler >> 8) & 0xff );
    zlibBlock.push_back( adler & 0xff );
    

    // the zlib block is now complete



    
    /*
    // check against real zlib implementation
    z_stream zStream;

    zStream.next_in = rawScanlineBytes;
    zStream.avail_in = numRawBytes;
    zStream.total_in = 0;
    
    int outSize = 2 * numRawBytes + 100;
    unsigned char *zlibOutBuffer = new unsigned char[ outSize ]; 
    zStream.next_out = zlibOutBuffer;
    zStream.avail_out = outSize;
    zStream.total_out = 0;

    zStream.data_type = Z_BINARY;

    zStream.zalloc = Z_NULL;
    zStream.zfree = Z_NULL;

    // init the stream
    // no compression
    int result;
    //result = deflateInit( &zStream, Z_DEFAULT_COMPRESSION);
    result = deflateInit( &zStream, Z_NO_COMPRESSION);

    if( result != Z_OK ) {
        printf( "zlib deflateInit error: %s\n", zStream.msg );
        }

    
    // deflate and flush
    result = deflate( &zStream, Z_FINISH );

    if( result != Z_STREAM_END ) {
        printf( "zlib deflate error (%d): %s\n", result, zStream.msg );
        }
    printf( "Total in = %d, total out = %d\n",
            zStream.total_in, zStream.total_out );
    
    

    
    printf( "Our raw bytes (%d):\n", numRawBytes );
    int b;
    for( b=0; b<numRawBytes; b++ ) {
        printf( "%02x ", rawScanlineBytes[ b ] );
        }
    
    printf( "\n\nOur zlib bytes (%d) (adler = %d) :\n",
            zlibBlock.size(), adler );
    
    for( b=0; b<zlibBlock.size(); b++ ) {
        printf( "%02x ", *( zlibBlock.getElement( b ) ) );
        }


    printf( "\n\nTheir zlib bytes (%d) (adler = %d) :\n",
            (int)( zStream.total_out ), zStream.adler );
    for( b=0; b<zStream.total_out; b++ ) {
        printf( "%02x ", zlibOutBuffer[ b ] );
        }

    int minBytes = zStream.total_out;
    if( minBytes > zlibBlock.size() ) {
        minBytes = zlibBlock.size();
        }

    for( b=0; b<minBytes; b++ ) {
        if( zlibOutBuffer[b] != *( zlibBlock.getElement( b ) ) ) {
            printf( "mismatch at byte %d\n", b );
            printf( "Theirs: %02x, ours: %02x\n", zlibOutBuffer[b],
                    *( zlibBlock.getElement( b ) ) );
            }
        }

    
    printf( "\n\n" );
    

    
    
    delete [] zlibOutBuffer;
    result = deflateEnd( &zStream );

    if( result != Z_OK ) {
        printf( "zlib deflateEnd error (%d): %s\n", result, zStream.msg );
        }

    */    
    

    
    // the zlib block is the data of an IDAT chunk

    unsigned char *zlibBytes = zlibBlock.getElementArray();

    writeChunk( "IDAT", zlibBytes, zlibBlock.size(), inStream );
    
    
    delete [] zlibBytes;
    
    // no data in end chunk
    writeChunk( "IEND", NULL, 0, inStream );


    delete [] rawScanlineBytes;

    
    // done
	}



Image *PNGImageConverter::deformatImage( InputStream *inStream ) {

    printf( "ERROR:  reading PNG images is not supported by "
            "PNGImageConverter\n" );

    return NULL;
	}
