/*----------------------------------------------------------------------------
.AVI file dumper
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: March 3, 2007
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <stdarg.h> /* va_list, va_start(), va_end() */
#include <setjmp.h> /* jmp_buf, setjmp(), longjmp() */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#if 0
/* C99 fixed-width types */
#include <stdint.h>
#else
typedef unsigned long	uint32_t;
#endif

typedef enum
{
	TT_OTHER = 0,	/* unknown or uninitialized track */
	TT_VIDEO,	/* video track */
	TT_AUDIO,	/* audio track */
	TT_MAX		/* (not a track; used to set array sizes) */
} track_type_t;

typedef struct
{
	unsigned long count;
	track_type_t type;
} avi_track_t;

typedef struct
{
	unsigned num_tracks;
	avi_track_t *track;
/* chunk offsets in "idx1" chunk -- are they relative to the start
of the file or the start of the "movi" chunk? */
//	unsigned long wad_delta;
//	char wad_delta_valid;
/* this field is used to pass info from "strh" to "strf" chunk */
	track_type_t track_type;
/* wad number of current wad in "idx1" chunk */
//	unsigned long curr_wad;
/* file offsets of "movi" and "idx1" chunks */
	unsigned long movi_offset;
	unsigned long idx1_offset;
} avi_t;

typedef union
{
	uint32_t n; /* numeric */
	char s[4]; /* string ("fourcc") */
} codec_t;

typedef struct
{
	unsigned char r, g, b;
} rgb_t;

typedef struct
{
	FILE *f;
	jmp_buf oops;
/* FILE DECODE INFO */
	void *file_parse_state;
/* AUDIO DECODE INFO */
	codec_t audio_codec;
	unsigned rate, adepth, channels;
/* VIDEO DECODE INFO */
	codec_t video_codec;
	unsigned wd, ht, vdepth;
	rgb_t *pal;
} state_t;
/*****************************************************************************
*****************************************************************************/
static unsigned read_le16(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] + buf[1] * 0x100;
}
/*****************************************************************************
CAUTION: the 'u' after '0x100' is significant
*****************************************************************************/
static unsigned long read_le32(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] +	buf[1] * 0x100u +
		buf[2] * 0x10000L + buf[3] * 0x1000000L;
}
/*****************************************************************************
*****************************************************************************/
static void oops(state_t *state, const char *fmt, ...)
{
	static char msg[128];
/**/
	va_list args;

	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);
	longjmp(state->oops, (int)msg);
}
/*****************************************************************************
*****************************************************************************/
static void read_or_die(state_t *state, char *buf, unsigned count)
{
	if(fread(buf, 1, count, state->f) != count)
		oops(state, "read_or_die: unexpected end-of-file");
}
/*****************************************************************************
*****************************************************************************/
static void display_codec(codec_t *c)
{
	if(isalnum(c->s[0]))
		printf("'%-4.4s'", c->s);
	else
		printf("#0x%lX", c->n);
}
/*****************************************************************************
*****************************************************************************/
static void recurse(state_t *state, unsigned long max_pos, int indent)
{
	unsigned long pos, len;
	avi_track_t *track;
	char buf[48];
	unsigned i;
	avi_t *avi;

	avi = (avi_t *)(state->file_parse_state);
	while(1)
	{
		pos = ftell(state->f);
		if(pos >= max_pos)
			return;
		read_or_die(state, buf, 8);
/* read chunk length; make sure it's even */
		len = read_le32(buf + 4);
		if(len & 1)
			len++;
		printf("%-.*s", indent, "                         ");
		if(!strnicmp(buf + 0, "LIST", 4))
		{
			read_or_die(state, buf + 8, 4);
			printf("pos=%7lu, chunk=LIST:%-4.4s, len=%lu\n",
				pos, buf + 8, len);
		}
		else
			printf("pos=%7lu, chunk=%-4.4s, len=%lu\n",
				pos, buf + 0, len);
		pos += 8;
/* "LIST" chunk... */
		if(!strnicmp(buf + 0, "LIST", 4))
		{
//			read_or_die(state, buf, 4);
/* ...recurse except for "movi" list */
			if(strnicmp(buf + 8, "movi", 4))
				recurse(state, pos + len, indent + 2);
			else
				avi->movi_offset = pos;
		}
/* "strh" chunk (AVISTREAMHEADER):
 0 FOURCC fcc;			="strh"
 4 DWORD  cb;			=length of this chunk, excluding fcc & cb

 0 FOURCC fccType;		=stream type: "auds", "mids", "txts", "vids"
 4 FOURCC fccHandler;		codec. Ignore this; get codec from
				  BITMAPINFO[HEADER] and WAVEFORMATEX)
 8 DWORD  dwFlags;		StreamDisabled, HasPaletteChanges
12 WORD   wPriority;
14 WORD   wLanguage;
16 DWORD  dwInitialFrames;
20 DWORD  dwScale;		frames/sec = dwRate/dwScale
24 DWORD  dwRate;
28 DWORD  dwStart;		in units of dwRate/dwScale
32 DWORD  dwLength;		in units of dwRate/dwScale
36 DWORD  dwSuggestedBufferSize; largest chunk size
40 DWORD  dwQuality;
44 DWORD  dwSampleSize;		need this? =0 if variable. In this case,
				  each sample must be in a different chunk
(other uninteresting fields)
48 */
		else if(!strnicmp(buf + 0, "strh", 4))
		{
			read_or_die(state, buf, 48);
			track = (avi_track_t *)realloc(avi->track,
				sizeof(avi_track_t) * (avi->num_tracks + 1));
			if(track == NULL)
				oops(state, "recurse: out of memory");
			avi->track = track;
			track = &avi->track[avi->num_tracks];
			avi->num_tracks++;
			if(!strnicmp(buf + 0, "auds", 4))
			{
				track->type = TT_AUDIO;
/* xxx - for PCM audio, this values is in units of SAMPLES.
I want it to be in units of BLOCKS. */
				track->count = read_le32(buf + 32);
				avi->track_type = TT_AUDIO;
			}
			else if(!strnicmp(buf + 0, "vids", 4))
			{
				track->type = TT_VIDEO;
				track->count = read_le32(buf + 32);
				avi->track_type = TT_VIDEO;
			}
			else
			{
				track->type = TT_OTHER;
				track->count = read_le32(buf + 32);
				avi->track_type = TT_OTHER;
			}
		}
/* get audio and video info from 'strf' chunk, which is either... */
		else if(!strnicmp(buf + 0, "strf", 4))
		{
			if(avi->track_type == TT_AUDIO)
			{
			read_or_die(state, buf, 20);
/* ...WAVEFORMATEX...
 0 WORD  wFormatTag;
 2 WORD  nChannels;
 4 DWORD nSamplesPerSec;
 8 DWORD nAvgBytesPerSec;
12 WORD  nBlockAlign;
14 WORD  wBitsPerSample;
(other uninteresting fields)
16 */
				state->audio_codec.n = read_le16(buf + 0);
				state->channels = read_le16(buf + 2);
				state->rate = (unsigned)read_le32(buf + 4);
				state->adepth = read_le16(buf + 14);
			}
			else if(avi->track_type == TT_VIDEO)
			{
				read_or_die(state, buf, 40);
/* ...or BITMAPINFO:
 0 DWORD  biSize;
 4 LONG   biWidth;
 8 LONG   biHeight;
12 WORD   biPlanes;
14 WORD   biBitCount;
16 DWORD  biCompression;
(other uninteresting fields)
40 */
				state->wd = (unsigned)read_le32(buf + 4);
				state->ht = (unsigned)read_le32(buf + 8);
				state->vdepth = read_le16(buf + 14);
				strncpy(state->video_codec.s, buf + 16, 4);
/* read palette */
				if(state->vdepth == 8)
				{
					state->pal = (rgb_t *)malloc(
						256 * sizeof(rgb_t));
					if(state->pal == NULL)
						oops(state, "recurse: "
							"no memory for "
							".AVI palette\n");
					for(i = 0; i < 256; i++)
					{
						read_or_die(state, buf, 4);
						state->pal[i].b = buf[0];
						state->pal[i].g = buf[1];
						state->pal[i].r = buf[2];
					}
				}
			}
		}
/* index */
		else if(!strnicmp(buf + 0, "idx1", 4))
			avi->idx1_offset = pos;
		pos += len;
		fseek(state->f, pos, SEEK_SET);
	}
}
/*****************************************************************************
*****************************************************************************/
static void destroy_avi(void *file_parse_state)
{
	avi_t *avi;

	avi = (avi_t *)file_parse_state;
	if(avi->track != NULL)
		free(avi->track);
	memset(avi, 0, sizeof(avi_t));
	free(avi);
}
/*****************************************************************************
*****************************************************************************/
static int validate_avi_file(state_t *state)
{
	unsigned char buf[12];
	unsigned long len;
	avi_t *avi;
	int i;

	fseek(state->f, 0, SEEK_SET);
/* read and validate AVI file headers */
	if(fread(buf, 1, 12, state->f) != 12)
		return +1; /* short file is not .AVI */
	if(memcmp(buf + 0, "RIFF", 4) || memcmp(buf + 8, "AVI ", 4))
		return +1;
/* it's AVI -- any badness from here is an error */
	len = read_le32(buf + 4);
/* allocate zeroed file parser state */
	avi = (avi_t *)calloc(1, sizeof(avi_t));
	if(avi == NULL)
	{
		printf("Error in validate_avi_file: "
			"no memory for .AVI file parser\n");
		return -1;
	}
	state->file_parse_state = avi;
/* set up error trapping for .AVI file parser */
	i = setjmp(state->oops);
	if(i != 0)
	{
		destroy_avi(avi);
		state->file_parse_state = NULL;
		printf("Error in %s\n", (char *)i);
		return -1;
	}
	recurse(state, len, 0);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static void destroy_state(state_t *state)
{
	if(state->file_parse_state != NULL)
		destroy_avi(state->file_parse_state);
	memset(state, 0, sizeof(state_t));
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	state_t state;
	int i;

/* check args */
	if(arg_c != 2)
	{
		printf(".AVI file dumper\n");
		return 1;
	}
/* open input file */
	memset(&state, 0, sizeof(state));
	state.f = fopen(arg_v[1], "rb");
	if(state.f == NULL)
	{
		printf("Error: can't open input file '%s'\n", arg_v[1]);
		return 1;
	}
printf("\nFile '%s'\n", arg_v[1]);
/* set up error trapping for file parsing */
	i = setjmp(state.oops);
	if(i != 0)
	{
		printf("%s\n", (char *)i);
		fclose(state.f);
		destroy_state(&state);
		return 1;
	}
	(void)validate_avi_file(&state);
/* display info */
	printf("VIDEO: Codec is ");
	display_codec(&state.video_codec);
	printf(", %ux%ux%u\n", state.wd, state.ht, state.vdepth);

	printf("AUDIO: Codec is ");
	display_codec(&state.audio_codec);
	printf(", %u samples/sec, %u bits/sample, %u %s\n",
		state.rate, state.adepth, state.channels,
		(state.channels > 1) ? "channels" : "channel");

	fclose(state.f);
	destroy_state(&state);
	return 0;
}
