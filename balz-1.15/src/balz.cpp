// balz.cpp is in the public domain
#define _CRT_DISABLE_PERFCRIT_LOCKS // for vc8 and later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// common data types
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef long long LONGLONG;

static FILE *in; // input file stream
static FILE *out; // output file stream

// a predictor estimates the probability of bit 1
// we keep two probabilities:
// one with fast update and one with slow update
// the final probability is a sum of both
class TPredictor {
private:
  WORD p1;
  WORD p2;

public:
  TPredictor(): p1(1<<15), p2(1<<15) {} // initially, p1 and p2 = 0.5

  int P() const {
    return (p1+p2); // combine two probabilities
  }

  void Update(int y) { // update p1 and p2, according to bit y
    if (y) {
      p1+=(WORD(~p1)>>3); // fast update
      p2+=(WORD(~p2)>>6); // slow update
    }
    else {
      p1-=(p1>>3); // fast update
      p2-=(p2>>6); // slow update
    }
  }
};

// arithmetic encoder and decoder derived from matt mahoney's fpaq0
class TEncoder {
private:
  DWORD x1;
  DWORD x2;

public:
  TEncoder(): x1(0), x2(-1) {} // range, initially [0, 1]

  void Encode(TPredictor &predictor, int y) { // encode bit y
    // calculate subrange with 64-bit precision
    const DWORD xmid=x1+((LONGLONG(x2-x1)*predictor.P())>>17);

    // update the range
    if (y)
      x2=xmid;
    else
      x1=xmid+1;

    predictor.Update(y); // update the predictor

    // write leading bytes of the range as they become known
    while ((x1^x2)<(1<<24)) {
      putc(x2>>24, out);
      x1<<=8;
      x2=(x2<<8)+255;
    }
  }

  void Flush() { // flush the encoder
    for (int i=0; i<4; i++) {
      putc(x2>>24, out);
      x2<<=8;
    }
  }
};

class TDecoder {
private:
  DWORD x1;
  DWORD x2;
  DWORD x;

public:
  TDecoder(): x1(0), x2(-1) {} // range, initially [0, 1]

  void Init() { // initialize the decoder
    for (int i=0; i<4; i++)
      x=(x<<8)+getc(in);
  }

  int Decode(TPredictor &predictor) { // decode one bit
    // calculate subrange with 64-bit precision
    const DWORD xmid=x1+((LONGLONG(x2-x1)*predictor.P())>>17);

    // update the range
    int y=(x<=xmid);
    if (y)
      x2=xmid;
    else
      x1=xmid+1;

    predictor.Update(y); // update the predictor

    // write leading bytes of the range as they become known
    while ((x1^x2)<(1<<24)) { 
      x1<<=8;
      x2=(x2<<8)+255;
      x=(x<<8)+getc(in);
    }

    return (y);
  }
};

#define TABBITS 7
#define TABSIZE (1<<TABBITS) // table size

// ppm - a special class for lz-output encoding
static class TPPM {
private:
  TPredictor p1[256][512];
  TPredictor p2[256][TABSIZE];

public:
  TEncoder encoder;
  TDecoder decoder;

  // encode literal/match length as a 9-bit symbol:
  // 0..255 - literals
  // 256..511 - match lengths
  // c - order-1 context (high byte of a rolz* context or buf[i-1])
  // *rolz is short for reduced offset lempel-ziv - the algorithm of balz
  void Encode(int s, int c) { 
    for (int i=8, j=1; i>=0; i--) {
      const int y=(s>>i)&1;
      encoder.Encode(p1[c][j], y);
      j+=(j+y);
    }
  }

  // encode match index
  // c - order-1 context (low byte of a rolz context or buf[i-2])
  void EncodeIndex(int x, int c) {
    for (int i=TABBITS-1, j=1; i>=0; i--) {
      const int y=(x>>i)&1;
      encoder.Encode(p2[c][j], y);
      j+=(j+y);
    }
  }

  int Decode(int c) { // decode literal/match length
    int s=1;
    while ((s+=(s+decoder.Decode(p1[c][s])))<512);

    return (s-512);
  }

  int DecodeIndex(int c) { // decode match index
    int x=1;
    while ((x+=(x+decoder.Decode(p2[c][x])))<TABSIZE);

    return (x-TABSIZE);
  }
} ppm;

#define MINMATCH 3 // smallest allowable match length
#define MAXMATCH (255+MINMATCH) // largest allowable match length

#define BUFBITS 25
#define BUFSIZE (1<<BUFBITS) // buffer size

static BYTE buf[BUFSIZE+MAXMATCH]; // data buffer
static DWORD tab[1<<16][TABSIZE]; // offset table

#define IDENT 0xba // identification byte

// perform a special transformation for executable data
static void exetransform(int y, int n) {
  const int end=n-8;
  int i=0;
  // search for pe file header
  while ((reinterpret_cast<int&>(buf[i])!=0x4550)&&(++i<end));

  // perform call/jmp address translation
  while (i<end) {
    if ((buf[i++]&254)==0xe8) {
      int &addr=reinterpret_cast<int&>(buf[i]);
      if (y) {
        if ((addr>=-i)&&(addr<(n-i)))
          addr+=i;
        else if ((addr>0)&&(addr<n))
          addr-=n;
      }
      else {
        if (addr<0) {
          if ((addr+i)>=0)
            addr+=n;
        }
        else if (addr<n)
          addr-=i;
      }
      i+=4;
    }
  }
}

// get match price based on match length and index
// if match is not long enough - return constant literal price
static inline int getprice(int len, int x) {
  return (len>=MINMATCH?(len<<TABBITS)-x:((MINMATCH-1)<<TABBITS)-8);
}

// get hash of three bytes using multiplicative hashing
static inline DWORD gethash(int i) {
  return (((reinterpret_cast<DWORD&>(buf[i])&0xffffff)*2654435761u)>>BUFBITS);
}

// get match/literal price at i
static int getmatch(int i, int n) {
  int len=MINMATCH-1; // match length
  int index=TABSIZE; // match index

  // determine search bound
  int end=i+MAXMATCH;
  if (end>n)
    end=n;

  const int c=reinterpret_cast<WORD&>(buf[i-2]); // rolz context
  const DWORD hash=gethash(i); // hash of current three bytes

  // search for the longest match at each offset stored
  for (int x=0; x<TABSIZE; x++) {
    const DWORD d=tab[c][x];
    if (!d)
      break;
    if ((d>>BUFBITS)!=hash) // check for valid hash
      continue;

    int p=d&(BUFSIZE-1);
    if (buf[p+len]!=buf[i+len]) // preliminary match test
      continue;

    // search for a longer match
    int j=i;
    while ((buf[p++]==buf[j])&&(++j<end));

    // a longer match found
    if ((j-=i)>len) {
      index=x; // set new match index
      if ((i+(len=j))>=end) // set new match length
        break;
    }
  }

  return (getprice(len, index)); // return price
}

// encode in to out
// compressed file format:
// 1-byte - identification byte (0xba)
// 8-bytes - uncompressed size
// ?-bytes - compressed data
static void encode(int max) {
  // a table for storing optimal match indexes for each match length
  BYTE idx[MAXMATCH+1];

  LONGLONG size=0; // uncompressed size
  // reserve some space for file header
  for (int i=0; i<(1+sizeof(size)); i++)
    putc(0, out);

  printf("encoding...\n");

  int n;
  // fill the buffer with data
  while ((n=fread(&buf, 1, BUFSIZE, in))>0) {
    exetransform(1, n); // perform a special exe transformation

    memset(&tab, 0, sizeof(tab)); // reset the offset table

    int i=0;
    while ((i<2)&&(i<n)) // encode two raw literals
      ppm.Encode(buf[i++], 0);

    while (i<n) {
      int len=MINMATCH-1; // match length
      int index=TABSIZE; // match index

      // determine search bound
      int end=i+MAXMATCH;
      if (end>n)
        end=n;

      const int c=reinterpret_cast<WORD&>(buf[i-2]); // rolz context
      const DWORD hash=gethash(i); // hash of current three bytes

      // search for the longest match at each offset stored
      for (int x=0; x<TABSIZE; x++) {
        const DWORD d=tab[c][x];
        if (!d)
          break;
        if ((d>>BUFBITS)!=hash) // check for valid hash
          continue;

        int p=d&(BUFSIZE-1);
        if (buf[p+len]!=buf[i+len]) // preliminary match test
          continue;

        // search for a longer match
        int j=i;
        while ((buf[p++]==buf[j])&&(++j<end));

        // a longer match found
        if ((j-=i)>len) {
          // fill the index table starting at new match length down to a previous one
          // with just found index - it's an optimal one in this length range
          for (int k=j; k>len; k--)
            idx[k]=x;
          index=x; // set new match index

          if ((i+(len=j))>=end) // set new match length
            break;
        }
      }

      // perform the path optimization
      if (len>=MINMATCH) {
        // get sum of the current match price and a followed match/literal price
        int sum=getprice(len, index)+getmatch(i+len, n);

        // if we have a chance to get a better sum
        if (sum<getprice(len+MAXMATCH, 0)) {
          // determine optimization lookahead, depending on mode
          const int lookahead=max?len:2;

          for (int j=1; j<lookahead; j++) {
            const int tmp=getprice(j, idx[j])+getmatch(i+j, n);
            if (tmp>sum) { // a better sum found
              sum=tmp;
              len=j; // set new match length
            }
          }

          index=idx[len]; // keep optimal match index
        }
      }

      // add a new offset to the table
      for (int x=TABSIZE-1; x>0; x--)
        tab[c][x]=tab[c][x-1];
      tab[c][0]=(hash<<BUFBITS)+i; // hash in high bits, offset in low bits

      if (len>=MINMATCH) {
        ppm.Encode((256-MINMATCH)+len, buf[i-1]); // encode match length
        ppm.EncodeIndex(index, buf[i-2]); // encode match index
        i+=len;
      }
      else { // not long enough match
        ppm.Encode(buf[i], buf[i-1]); // encode a literal
        i++;
      }
    }

    size+=n; // update the size
  }

  ppm.encoder.Flush(); // flush the encoder

  rewind(out); // seek to the beginning

  putc(IDENT, out); // write identification byte
  fwrite(&size, 1, sizeof(size), out); // write uncompressed size
}

// decode in to out
static void decode() {
  BYTE cnt[1<<16]; // count for each context

  // check identification byte
  if (getc(in)!=IDENT) {
    fprintf(stderr, "bad file format\n");
    exit(1);
  }

  // read uncompressed size
  LONGLONG size=-1;
  fread(&size, 1, sizeof(size), in);
  if (size<0) {
    fprintf(stderr, "size error\n");
    exit(1);
  }

  printf("decoding...\n");

  ppm.decoder.Init(); // initialize the decoder

  while (size>0) {
    int i=0;
    while ((i<2)&&(i<size)) { // decode two raw literals
      const int s=ppm.Decode(0);
      // raw literals cannot be larger than 255
      if (s>=256) {
        fprintf(stderr, "data error\n");
        exit(1);
      }
      buf[i++]=s;
    }

    while ((i<BUFSIZE)&&(i<size)) {
      const int c=reinterpret_cast<WORD&>(buf[i-2]); // rolz context
      const int j=i; // keep current buffer offset
      const int s=ppm.Decode(buf[i-1]); // decode literal/match length

      if (s>=256) { // match
        int len=BYTE(s);
        // get the buffer offset from the offset table
        int p=tab[c][(cnt[c]-ppm.DecodeIndex(buf[i-2]))&(TABSIZE-1)];
        buf[i++]=buf[p++]; // partially unrolled loop
        buf[i++]=buf[p++];
        buf[i++]=buf[p++];
        while (len--) // copy the rest of a match
          buf[i++]=buf[p++];
      }
      else // literal
        buf[i++]=s;

      tab[c][++cnt[c]&(TABSIZE-1)]=j; // add a new offset
    }

    exetransform(0, i); // perform a special exe untransformation

    fwrite(&buf, 1, i, out); // write just decoded buffer
    size-=i;
  }
}

int main(int argc, char **argv) {
  printf("balz v1.15 by ilia muraviev\n");

  // check command-line arguments
  if ((argc!=4)||((argv[1][0]&254)!='d')) {
    fprintf(stderr, "usage: balz e[x]|d in out\n");
    exit(1);
  }

  // open file streams
  if (!(in=fopen(argv[2], "rb"))) {
    perror(argv[2]);
    exit(1);
  }
  // do not overwrite an existing file
  if (fopen(argv[3], "rb")) {
    fprintf(stderr, "cannot overwrite %s\n", argv[3]);
    exit(1);
  }
  if (!(out=fopen(argv[3], "wb"))) {
    perror(argv[3]);
    exit(1);
  }

  // encode or decode
  if (argv[1][0]&1)
    encode(argv[1][1]=='x'); // x - max mode
  else
    decode();

  // close file streams
  fclose(out);
  fclose(in);

  printf("done\n");

  return (0);
}
