/*----------------------------------------------------------------------------
Quicktime (.MOV) file dumper
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: March 3, 2007
This code is public domain (no copyright).
You can do whatever you want with it.

WARNING: QuickTime files with compressed headers (CMOV atom)
are not supported.
----------------------------------------------------------------------------*/
#include <stdarg.h> /* va_list, va_start(), va_end() */
#include <string.h> /* strncmp(), memset() */
#include <setjmp.h> /* jmp_buf, setjmp(), longjmp() */
#include <stdlib.h> /* realloc(), calloc(), free() */
#include <stdio.h> /* (lotsa stuff) */
#include <ctype.h> /* isalnum() */
#if 0
/* C99 fixed-width types */
#include <stdint.h>
#else
typedef unsigned long	uint32_t;
#endif

#if 1
#define	DEBUG(X)	X
#else
#define	DEBUG(X)	/* nothing */
#endif

/* QuickTime track type */
typedef enum
{
	MOV_TT_SMHD = 1,/* sound (audio) */
	MOV_TT_VMHD,	/* video */
	MOV_TT_NMHD,	/* ? */
	MOV_TT_GMHD 	/* ? */
} mov_track_type_t;

/* state for one track in a QuickTime file: */
typedef struct
{
/* QuickTime track type */
	mov_track_type_t type;
/* fields read from atoms in QuickTime file:
chunk offset info: */
	unsigned long stco_offset, stco_count;
/* sample-to-chunk info: */
	unsigned long stsc_offset, stsc_count;
/* sync (key) samples */
	unsigned long stss_offset, stss_count;
/* sample size info: */
	unsigned long stsz_offset, stsz_count, stsz;
/* time-to-sample conversion info */
	unsigned long stts_offset, stts_count;
/* Frame rate is usually variable. Display time of each frame is
    frame_display_time / quanta_per_sec
where frame_display_time comes from the "stts" atom,
and quanta_per_sec comes from the "mdhd" atom */
	unsigned long quanta_per_sec;
} mov_track_t;

/* QuickTime decode state */
typedef struct
{
	unsigned long mdat_start, mdat_end;
	unsigned num_tracks;
	mov_track_t *track;
} mov_t;

typedef union
{
	uint32_t n; /* numeric */
	char s[4]; /* string ("fourcc") */
} codec_t;

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
} state_t;
/*****************************************************************************
*****************************************************************************/
static unsigned read_be16(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] * 0x100 + buf[1];
}
/*****************************************************************************
CAUTION: the 'u' after '0x100' is significant
*****************************************************************************/
static unsigned long read_be32(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] * 0x1000000L + buf[1] * 0x10000L +
		buf[2] * 0x100u + buf[3];
}
/*****************************************************************************
*****************************************************************************/
static void oops(state_t *state, const char *fmt, ...)
{
	static char msg[128];
/**/
	va_list args;

	va_start(args, fmt);
/* maybe use vsnprintf() here if you have it... */
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
(DEBUG) exceptions thrown:
- bad track number
- unexpected end-of-file
*****************************************************************************/
static void dump_stco(state_t *state, unsigned track_num)
{
	mov_track_t *track;
	unsigned long i;
	char buf[4];
	mov_t *mov;

	mov = (mov_t *)(state->file_parse_state);
	if(track_num >= mov->num_tracks)
		oops(state, "dump_stco: bad track number "
			"%u (%u max)", track_num, mov->num_tracks - 1);
	track = &mov->track[track_num];
	printf("5 of the %lu chunk offsets for track %u:\n",
		track->stco_count, track_num);
	for(i = 0; i < track->stco_count; i++)
	{
		fseek(state->f, track->stco_offset + 4 * i, SEEK_SET);
		read_or_die(state, buf, 4);
		printf("%8lu  ", read_be32(buf + 0));
		if(i > 5)
		{
			printf("...");
			break;
		}
	}
	printf("\n");
}
/*****************************************************************************
(DEBUG) exceptions thrown:
- bad track number
- unexpected end-of-file
*****************************************************************************/
static void dump_stsz(state_t *state, unsigned track_num)
{
	mov_track_t *track;
	unsigned long i;
	char buf[4];
	mov_t *mov;

	mov = (mov_t *)(state->file_parse_state);
	if(track_num >= mov->num_tracks)
		oops(state, "dump_stsz: bad track number "
			"%u (%u max)", track_num, mov->num_tracks - 1);
	track = &mov->track[track_num];
	printf("5 of the %lu sample sizes for track %u:\n",
		track->stsz_count, track_num);
	if(track->stsz != 0)
	{
		printf("%8lu (fixed)\n", track->stsz);
		return;
	}
	for(i = 0; i < track->stsz_count; i++)
	{
		fseek(state->f, track->stsz_offset + 4 * i, SEEK_SET);
		read_or_die(state, buf, 4);
		printf("%8lu  ", read_be32(buf + 0));
		if(i > 5)
		{
			printf("...");
			break;
		}
	}
	printf("\n");
}
/*****************************************************************************
(DEBUG) exceptions thrown:
- bad track number
- unexpected end-of-file
*****************************************************************************/
static void dump_stsc(state_t *state, unsigned track_num)
{
	mov_track_t *track;
	unsigned long i;
	char buf[12];
	mov_t *mov;

	mov = (mov_t *)(state->file_parse_state);
	if(track_num >= mov->num_tracks)
		oops(state, "dump_stsc: bad track number "
			"%u (%u max)", track_num, mov->num_tracks - 1);
	track = &mov->track[track_num];
	printf("Sample-to-chunk info for track %u:\n", track_num);
	printf(	"Starting  Samples\n"
		"chunk     /chunk    ID\n"
		"--------  --------  --------\n");
	for(i = 0; i < track->stsc_count; i++)
	{
		fseek(state->f, track->stsc_offset + 12 * i, SEEK_SET);
		read_or_die(state, buf, 12);
		printf("%8lu  %8lu  %8lu\n", read_be32(buf + 0),
			read_be32(buf + 4), read_be32(buf + 8));
		if(i > 5)
		{
			printf("...\n");
			break;
		}
	}
}
/*****************************************************************************
moov->trak->mdia->minf->stbl
*****************************************************************************/
static void do_stbl(state_t *state, unsigned long max_pos)
{
	unsigned long pos, len, type;
	mov_track_t *track;
	char buf[68];
	mov_t *mov;

	mov = (mov_t *)(state->file_parse_state);
	if(mov->num_tracks == 0)
		oops(state, "do_stbl: got track info "
			"before smhd/vmhd/nmhd/gmhd");
	track = &mov->track[mov->num_tracks - 1];
	while(1)
	{
		pos = ftell(state->f);
		if(pos >= max_pos)
			return;
		read_or_die(state, buf, 8);
		len = read_be32(buf + 0);
		type = *(uint32_t *)(buf + 4);
		DEBUG(printf("        pos=%7lu, atom=%-4.4s, len=%lu\n",
			pos, buf + 4, len);)
		pos += len;
		if(type == *(uint32_t *)"stco")
		{
			read_or_die(state, buf, 8);
/*			if(buf[0] != 0)
				oops(state, "do_stbl: "
					"'stco' atom is version "
					"%u (should be 0)", buf[0]); */
			track->stco_offset = ftell(state->f);
			track->stco_count = read_be32(buf + 4);
/*			if(track->stco_count < 1)
				oops(state, "do_stbl: stco_count=0"); */
		}
		else if(type == *(uint32_t *)"stsc")
		{
			read_or_die(state, buf, 8);
/*			if(buf[0] != 0)
				oops(state, "do_stbl: "
				"'stsc' atom is version "
					"%u (should be 0)", buf[0]); */
			track->stsc_offset = ftell(state->f);
			track->stsc_count = read_be32(buf + 4);
/*			if(track->stsc_count < 1)
				oops(state, "do_stbl: stsc_count=0"); */
		}
		else if(type == *(uint32_t *)"stsd")
		{
			unsigned long i;

			read_or_die(state, buf, 24);
/*			if(buf[0] != 0)
				oops(state, "do_stbl: "
					"'stsd' atom is version "
					"%u (should be 0)", buf[0]); */
			i = read_be32(buf + 4);
			if(i != 1)
				oops(state, "do_stbl: multiple "
					"(%lu) 'stsd' entries per track "
					"not supported", i);
/* video info */
			if(track->type == MOV_TT_VMHD)
			{
				state->video_codec.n = *(uint32_t *)(buf + 12);//4);
/*				if(len < 100)
					oops(state, "do_stbl: "
						"video 'stsd' atom is "
						"is too short (%lu bytes, "
						"must be >=100)", len); */
				read_or_die(state, buf, 68);
				state->wd = read_be16(buf + 16);
				state->ht = read_be16(buf + 18);
				state->vdepth = read_be16(buf + 66);
			}
/* audio info */
			else if(track->type == MOV_TT_SMHD)
			{
				state->audio_codec.n = *(uint32_t *)(buf + 12);
/*				if(len < 52)
					oops(state, "do_stbl: "
						"Audio 'stsd' atom is "
						"too short (%lu bytes, "
						"must be >=52)", len); */
				read_or_die(state, buf, 20);
				state->rate = read_be16(buf + 16);
				state->adepth = read_be16(buf + 10);
				state->channels = read_be16(buf + 8);
			}
		}
		else if(type == *(uint32_t *)"stss")
		{
			read_or_die(state, buf, 8);
/*			if(buf[0] != 0)
				oops(state, "do_stbl: "
					"'stss' atom is version "
					"%u (should be 0)", buf[0]); */
			track->stss_offset = ftell(state->f);
			track->stss_count = read_be32(buf + 4);
/*			if(track->stss_count < 1)
				oops(state, "do_stbl: stss_count=0"); */
		}
		else if(type == *(uint32_t *)"stsz")
		{
			read_or_die(state, buf, 12);
/*			if(buf[0] != 0)
				oops(state, "do_stbl: "
				"'stsz' atom is version "
					"%u (should be 0)", buf[0]); */
			track->stsz_offset = ftell(state->f);
			track->stsz = read_be32(buf + 4);
			track->stsz_count = read_be32(buf + 8);
/*			if(track->stsz_count < 1)
				oops(state, "do_stbl: stsz_count=0");
			if(track->type == MOV_TT_VMHD)
				state->num_frames = track->stsz_count; */
		}
		else if(type == *(uint32_t *)"stts")
		{
			read_or_die(state, buf, 8);
/*			if(buf[0] != 0)
				oops(state, "do_stbl: "
				"'stts' atom is version "
					"%u (should be 0)", buf[0]); */
			track->stts_offset = ftell(state->f);
			track->stts_count = read_be32(buf + 4);
/*			if(track->stts_count < 1)
				oops(state, "do_stbl: stts_count=0"); */
/* the video frame rate is USUALLY variable but not always... */
			DEBUG(
				read_or_die(state, buf + 8, 16);
				if(track->stts_count == 1 ||
					(track->stts_count == 2 &&
					read_be32(buf + 12) ==
					read_be32(buf + 20)))
				{
					printf("Fixed sample rate: "
						"%lu samples/sec\n",
						track->quanta_per_sec /
						read_be32(buf + 12));
				}
			)
		}
		else
			oops(state, "do_stbl: unrecognized "
				"QuickTime atom '%-4.4s'", buf + 4);
		fseek(state->f, pos, SEEK_SET);
	}
}
/*****************************************************************************
moov->trak->mdia->minf
*****************************************************************************/
static void do_minf(state_t *state, unsigned long max_pos)
{
	unsigned long pos, len, type;
	mov_track_t *track;
	char buf[8];
	mov_t *mov;

	mov = (mov_t *)(state->file_parse_state);
	if(mov->num_tracks == 0)
		oops(state, "do_minf: got track info "
			"before smhd/vmhd/nmhd/gmhd");
	track = &mov->track[mov->num_tracks - 1];
	while(1)
	{
		pos = ftell(state->f);
		if(pos >= max_pos)
			break;
		read_or_die(state, buf, 8);
		len = read_be32(buf + 0);
		type = *(uint32_t *)(buf + 4);
		DEBUG(printf("        pos=%7lu, atom=%-4.4s, len=%lu\n",
			pos, buf + 4, len);)
		pos += len;
		if(type == *(uint32_t *)"smhd")
		{
			DEBUG(printf("Current track is now AUDIO\n");)
			track->type = MOV_TT_SMHD;
		}
		else if(type == *(uint32_t *)"vmhd")
		{
			DEBUG(printf("Current track is now VIDEO\n");)
			track->type = MOV_TT_VMHD;
		}
		else if(type == *(uint32_t *)"nmhd")
		{
			track->type = MOV_TT_NMHD;
			DEBUG(printf("Current track is now NMHD\n");)
		}
		else if(type == *(uint32_t *)"gmhd")
		{
			track->type = MOV_TT_GMHD;
			DEBUG(printf("Current track is now GMHD\n");)
		}
		else if(type == *(uint32_t *)"hdlr")
			/* nothing */;
		else if(type == *(uint32_t *)"dinf")
			/* nothing */;
		else if(type == *(uint32_t *)"stbl")
			do_stbl(state, pos);
		else
			oops(state, "do_minf: unrecognized "
				"QuickTime atom '%-4.4s'", buf + 4);
		fseek(state->f, pos, SEEK_SET);
	}
}
/*****************************************************************************
moov->trak->mdia
*****************************************************************************/
static void do_mdia(state_t *state, unsigned long max_pos)
{
	unsigned long pos, len, type;
	mov_track_t *track;
	char buf[16];
	mov_t *mov;

	mov = (mov_t *)(state->file_parse_state);
/* allocate a new track */
	track = (mov_track_t *)realloc(mov->track,
		sizeof(mov_track_t) * (mov->num_tracks + 1));
	if(track == NULL)
		oops(state, "do_mdia: out of memory");
	mov->track = track;
	track = &mov->track[mov->num_tracks];
	mov->num_tracks++;
	while(1)
	{
		pos = ftell(state->f);
		if(pos >= max_pos)
			return;
		read_or_die(state, buf, 8);
		len = read_be32(buf + 0);
		type = *(uint32_t *)(buf + 4);
		DEBUG(printf("      pos=%7lu, atom=%-4.4s, len=%lu\n",
			pos, buf + 4, len);)
		pos += len;
		if(type == *(uint32_t *)"mdhd")
		{
			read_or_die(state, buf, 16);
/*			if(buf[0] != 0)
				oops(state, "do_mdia: "
					"'mdhd' atom is version "
					"%u (should be 0)", buf[0]); */
			track->quanta_per_sec = read_be32(buf + 12);
			DEBUG(printf("%lu time quanta/sec\n",
				track->quanta_per_sec);)
		}
		else if(type == *(uint32_t *)"hdlr")
			/* nothing */;
		else if(type == *(uint32_t *)"minf")
			do_minf(state, pos);
		else
			oops(state, "do_mdia: unrecognized "
				"QuickTime atom '%-4.4s'", buf + 4);
		fseek(state->f, pos, SEEK_SET);
	}
}
/*****************************************************************************
moov->trak
*****************************************************************************/
static void do_trak(state_t *state, unsigned long max_pos)
{
	unsigned long pos, len, type;
	char buf[8];

	while(1)
	{
		pos = ftell(state->f);
		if(pos >= max_pos)
			return;
		read_or_die(state, buf, 8);
		len = read_be32(buf + 0);
		type = *(uint32_t *)(buf + 4);
		DEBUG(printf("    pos=%7lu, atom=%-4.4s, len=%lu\n",
			pos, buf + 4, len);)
		pos += len;
		if(type == *(uint32_t *)"tkhd")
			/* nothing */;
		else if(type == *(uint32_t *)"tref")
			/* nothing */;
		else if(type == *(uint32_t *)"edts")
			/* nothing */;
		else if(type == *(uint32_t *)"load")
			/* nothing */;
		else if(type == *(uint32_t *)"udta")
			/* nothing */;
		else if(type == *(uint32_t *)"mdia")
			do_mdia(state, pos);
#if 0
/* .mp4 file with .mov extension has 'tapt' atom here */
		else
			oops(state, "do_trak: unrecognized "
				"QuickTime atom '%-4.4s'", buf + 4);
#endif
		fseek(state->f, pos, SEEK_SET);
	}
}
/*****************************************************************************
cmov
*****************************************************************************/
static void do_cmov(state_t *state, unsigned long max_pos)
{
	unsigned long pos, len, type;
	char buf[8];

	while(1)
	{
		pos = ftell(state->f);
		if(pos >= max_pos)
			return;
		read_or_die(state, buf, 8);
		len = read_be32(buf + 0);
		type = *(uint32_t *)(buf + 4);
		DEBUG(printf("    pos=%7lu, atom=%-4.4s, len=%lu\n",
			pos, buf + 4, len);)
		pos += len;
		if(type == *(uint32_t *)"dcom")
		{
			read_or_die(state, buf, 4);
			if(strncmp(buf, "zlib", 4))
				oops(state, "do_cmov: header "
					"compression method '%-4.4s' "
					"not supported", buf);
		}
		else if(type == *(uint32_t *)"cmvd")
		{
			read_or_die(state, buf, 4);
			// ### - decompressed size = read_be32(buf + 0)
			// ### - do 'inflate' (gzip/zlib) decompression
			// and feed decompressed stream to do_moov()
		}
		else
			oops(state, "do_cmov: unrecognized "
				"QuickTime atom '%-4.4s'", buf + 4);
		fseek(state->f, pos, SEEK_SET);
	}
}
/*****************************************************************************
moov
*****************************************************************************/
static void do_moov(state_t *state, unsigned long max_pos)
{
	unsigned long pos, len, type;
	char buf[8];

	while(1)
	{
		pos = ftell(state->f);
		if(pos >= max_pos)
			return;
		read_or_die(state, buf, 8);
		len = read_be32(buf + 0);
		type = *(uint32_t *)(buf + 4);
		DEBUG(printf("  pos=%7lu, atom=%-4.4s, len=%lu\n",
			pos, buf + 4, len);)
		pos += len;
		if(type == *(uint32_t *)"mvhd")
			/* nothing */;
		else if(type == *(uint32_t *)"cmov")
			do_cmov(state, pos);
		else if(type == *(uint32_t *)"udta")
			/* nothing */;
		else if(type == *(uint32_t *)"trak")
			do_trak(state, pos);
		else if(type == *(uint32_t *)"free")
			/* nothing */;
		else if(type == *(uint32_t *)"wide")
			/* nothing */;
		else
			oops(state, "do_moov: unrecognized "
				"QuickTime atom '%-4.4s'", buf + 4);
		fseek(state->f, pos, SEEK_SET);
	}
}
/*****************************************************************************
*****************************************************************************/
static void destroy(void *file_parse_state)
{
	mov_t *mov;

	mov = (mov_t *)file_parse_state;
	if(mov->track != NULL)
		free(mov->track);
	memset(mov, 0, sizeof(mov_t));
	free(mov);
}
/*****************************************************************************
*****************************************************************************/
static int validate_mov_file(state_t *state)
{
	unsigned long pos, len, type;
	volatile int i;
	char buf[8];
	unsigned j;
	mov_t *mov;

/* allocate zeroed file parser state */
	mov = (mov_t *)calloc(1, sizeof(mov_t));
	if(mov == NULL)
	{
		printf("Error in validate_mov_file: "
			"no memory for .MOV file parser\n");
		return -1;
	}
	state->file_parse_state = mov;
/* set up error trapping for QuickTime file parser */
	i = setjmp(state->oops);
	if(i != 0)
	{
		destroy(mov);
		state->file_parse_state = NULL;
		printf("Error in %s\n", (char *)i);
		return -1;
	}
	fseek(state->f, 0, SEEK_SET);
	while(1)
	{
		pos = ftell(state->f);
		if(fread(buf, 1, 8, state->f) != 8)
		{
/* EOF means success -- prepare to return */
			return 0;
		}
		len = read_be32(buf + 0);
		type = *(uint32_t *)(buf + 4);
		DEBUG(printf("pos=%7lu, atom=%-4.4s, len=%lu\n",
			pos, buf + 4, len);)
		pos += len;
		if(type == *(uint32_t *)"moov")
			do_moov(state, pos);
		else if(type == *(uint32_t *)"mdat")
		{
			mov->mdat_start = ftell(state->f) + 16;
			mov->mdat_end = mov->mdat_start + len;
		}
		else if(type == *(uint32_t *)"free")
			/* nothing */;
		else if(type == *(uint32_t *)"wide")
			/* nothing */;
#if 0
/* .mp4 file with .mov extension has 'ftyp' atom at top level */
		else
		{
			destroy(mov);
			state->file_parse_state = NULL;
/* bad atom at top level could mean this is not a QuickTime file,
so return +1 */
			return +1;
		}
#endif
		fseek(state->f, pos, SEEK_SET);
	}
}
/*****************************************************************************
*****************************************************************************/
static void destroy_mov(void *file_parse_state)
{
	mov_t *mov;

	mov = (mov_t *)file_parse_state;
	if(mov->track != NULL)
		free(mov->track);
	memset(mov, 0, sizeof(mov_t));
	free(mov);
}
/*****************************************************************************
*****************************************************************************/
static void destroy_state(state_t *state)
{
	if(state->file_parse_state != NULL)
		destroy_mov(state->file_parse_state);
	memset(state, 0, sizeof(state_t));
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
int main(int arg_c, char *arg_v[])
{
	state_t state;
	int i;

/* check args */
	if(arg_c != 2)
	{
		printf("QuickTime (.MOV) file dumper\n");
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
	(void)validate_mov_file(&state);
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
