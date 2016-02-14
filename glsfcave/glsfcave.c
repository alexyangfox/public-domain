/* GLSFcave - glsfcave.c */
/* by TOMARI, Hisanobu */
/* UNIX/OpenGL port of SFcave by SunFlat */
#define GLSFCAVE_VERSION "1.1"

/* configurations */
#define DEFAULTTITLE "GLSFcave"
#define NUMTAILS 48
#define WINDOWSIZE 400,300
#define ACCEL 0.001f
#define TIMER_MS 12
#define SCOREBUFSIZE 32
#define DEVRANDOM "/dev/random"
#define HSFILE ".glsfcave"
#define HSBUFSIZE 64
#define BYTE 8
#define PADWIDTH 0.15f
#define PADDEPTH 3
#define PADFREQ 50

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <libgen.h> /* basename(3) */
#include <limits.h>
#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#ifdef GLSFCAVE_SOUND
#include <AL/alut.h>
#endif /* defined(GLSFCAVE_SOUND) */

#define ESC '\033' /* keycode for escape key */
#define MIN(x,y) (((x)>(y))?(y):(x))
#define MAX(x,y) (((x)>(y))?(x):(y))
#define STATE_GAME 0x01
#define STATE_END 0x02
#define STATE_CRASH 0x04
#define RANDOM_MAX (0x7FFFFFFF)

float tails[NUMTAILS],center[3*NUMTAILS],pad[3*NUMTAILS];
float accel,speed;
unsigned int score,highscore,state;
int winwidth,winheight;
char hsfile[PATH_MAX];
#ifdef GLSFCAVE_SOUND
ALuint sndbuf,sndsrc;
int se=1; /*sound enable*/
#endif
static void display(void);
static void drawTails(void);
static void drawFlares(void);
static void drawWalls(void);
static void drawBar(float height);
static void drawScore(void);
static void init(void);
static void keyboard(unsigned char key,int x,int y);
static void mouse(int button,int mousestate,int x,int y);
static void periodic(int value);
static void resize(int w, int h);
static void syncHS(void);
static unsigned long seedRandom(void);
static float caveWidth(int score);
static void bye(void);
int main(int argc, char **argv);

static void display()
{
  glClear(GL_COLOR_BUFFER_BIT);

  /* draw tails */
  if(state&(STATE_GAME|STATE_CRASH))
    {
      drawTails();
      drawWalls();
    }

  if(state&STATE_CRASH)
    {
      glPushMatrix();
      glTranslatef(-1.0f/3.0f,tails[0],0.0f);
      drawFlares();
      glPopMatrix();
      state|=STATE_END;
    }

  drawScore();
  glutSwapBuffers();
}

static void drawTails()
{
  unsigned int i;

  glPushMatrix();
  glLoadIdentity();
  glTranslatef(-1.0f/3.0f,0.0f,0.0f);
  glScalef(-2.0f/3.0f/NUMTAILS,1.0f,1.0f);
  glColor3f(0.0f,1.0f,0.0f);

  for(i=NUMTAILS-2; i>0; i--)
    {
      unsigned int j=i;
      unsigned int k=i+1;

      glPushMatrix();
      glTranslatef(i,0.0f,0.0f);
      glBegin(GL_QUADS);
      	glVertex2f(1.0f,tails[k]);
  	glVertex2f(0.0f,tails[j]);
      	glVertex2f(3.0f,tails[j]);
      	glVertex2f(4.0f,tails[k]);
      glEnd();
      glBegin(GL_LINES);
      	glVertex2f(1.0f,tails[k]);
      	glVertex2f(0.0f,tails[j]);
      	glVertex2f(3.0f,tails[j]);
      	glVertex2f(4.0f,tails[k]);
      glEnd();
      glPopMatrix();
    }
  glPopMatrix();
}

static void drawWalls()
{
  unsigned int i;

  glPushMatrix(); /* upper */
  glTranslatef(-1.0f,1.0f,0.0f);
  glScalef(2.0f/(3.0f*((float)NUMTAILS)),-1.0f,0.0f);
  for(i=3*NUMTAILS; i>0; i--)
    {
      unsigned int j=i-1;
      drawBar(1.0-center[j]-caveWidth(score-j));
      glTranslatef(1.0f,0.0f,0.0f);
    }
  glPopMatrix();
  glPushMatrix(); /* lower */
  glTranslatef(-1.0f,-1.0f,0.0f);
  glScalef(2.0f/(3.0f*((float)NUMTAILS)),1.0f,0.0f);
  for(i=3*NUMTAILS; i>0; i--)
    {
      unsigned int j=i-1;
      int k=score-j;
      unsigned int l=k%PADFREQ;
      drawBar(1.0f+center[j]-caveWidth(k));

      if(k>=0 && l==0) /* Draw pads */
	{
	  glPushMatrix();
	  glTranslatef(0.0f,1.0f+pad[j],0.0f);
	  glColor3f(0.0f,1.0f,0.0f);
	  glBegin(GL_QUADS);
	  glVertex2f(0.0f,PADWIDTH);
	  glVertex2f(0.0f,-PADWIDTH);
	  glVertex2f(PADDEPTH,-PADWIDTH);
	  glVertex2f(PADDEPTH,PADWIDTH);
	  glEnd();
	  glPopMatrix();
	}
      glTranslatef(1.0f,0.0f,0.0f);
    }
  glPopMatrix();
}

static void drawBar(float height)
{
  glBegin(GL_QUADS);
  glColor3f(0.0f,0.0f,0.0f);
  glVertex2f(0.0f,0.0f);
  glVertex2f(1.0f,0.0f);
  glColor3f(0.0f,1.0f,0.0f);
  glVertex2f(1.0f,height);
  glVertex2f(0.0f,height);
  glEnd();
}

static void drawFlares()
{
  unsigned int i;
  for(i=360; i!=0; i--)
    {
      unsigned int color=random();
      glRotatef(1.0f,0,0,1);
      glColor3f((color&0x1)?1.0f:0.0f,(color&0x2)?1.0f:0.0f,(color&0x4)?1.0f:0.0f);
      glBegin(GL_LINES);
      glVertex2f(0.0f,0.0f);
      glVertex2f(((float) random())/((float) RANDOM_MAX),0.0f);
      glEnd();
    }
}

static void drawScore()
{
  char buf[SCOREBUFSIZE];
  int len=0;
  unsigned int i;

  if(state&STATE_GAME)
    len=snprintf(buf,SCOREBUFSIZE, "%u",score);
  else if(state==STATE_END)
    len=snprintf(buf,SCOREBUFSIZE, "HS: %u",highscore);
  else if(state&STATE_CRASH)
    len=snprintf(buf,SCOREBUFSIZE, "%u (HS: %u)",score,highscore);

  if(len>0)
    {
      len=MIN(SCOREBUFSIZE,len);
      int bitmapWidth=glutBitmapLength(GLUT_BITMAP_HELVETICA_18,(unsigned char *)buf);
      
      /* background */
      glPushMatrix(); 
      glTranslatef(-1.0f,-1.0f,0.0f);
      glScalef(2.0f/((float) winwidth),2.0f/((float) winheight),0.0f);
      glColor3f(0.0f,0.0f,0.0f);
      glBegin(GL_QUADS);
	glVertex2f(0.0f,18.0f);
	glVertex2f(0.0f,0.0f);
	glVertex2f(bitmapWidth,0.0f);
	glVertex2f(bitmapWidth,18.0f);
      glEnd();
      glPopMatrix();

      /* characters */
      glColor3f(1.0f,1.0f,1.0f);
      glRasterPos2f(-1.0f,-1.0f);
      for(i=0; i<len; i++)
	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,buf[i]);
    }
}

static void init()
{
  unsigned int i;
  for(i=0; i<NUMTAILS; i++)
    tails[i]=0.0f;
  for(i=0; i<3*NUMTAILS; i++)
    {
      center[i]=0.0f;
      pad[i]=0.0f;
    }
  state=STATE_GAME;
  accel=-ACCEL;
  speed=0.0f;
  score=0;
#ifdef GLSFCAVE_SOUND
  if(se)
    alSourcePlay(sndsrc);
#endif
  glutTimerFunc(TIMER_MS,&periodic,0);
}

static void syncHS()
{
  FILE *fh;
  unsigned int recordedHS;
  char buf[HSBUFSIZE];
  highscore=MAX(highscore,score);
  /* read */
  if((fh=fopen(hsfile,"r"))==NULL)
    {
      perror(hsfile);
      recordedHS=0;
    }
  else
    {
      int c;
      unsigned int i=0;
      while((c=fgetc(fh))!=EOF && i<HSBUFSIZE-1)
	buf[i++]=c;
      buf[i]=0;
      recordedHS=strtol(buf,NULL,10);
      fclose(fh);
    }
  highscore=MAX(recordedHS,highscore);
  if(highscore!=recordedHS)
    {
      /* write */
      if((fh=fopen(hsfile,"w+"))==NULL)
	perror(hsfile);
      else
	fprintf(fh,"%u\n",highscore);
    }
}

static void keyboard(key,x,y)
     unsigned char key;
     int x,y;
{
  switch (key)
    {
    case 'q':
    case 'Q':
    case ESC:
      exit(EXIT_SUCCESS);
      break;
#ifdef GLSFCAVE_SOUND
    case 's':
    case 'S':
      se=!se;
      if(se)
	alSourcePlay(sndsrc);
      else
	alSourceStop(sndsrc);
      break;
#endif /* defined(GLSFCAVE_SOUND) */
    default:
      break;
    }
}

static void mouse(button,mousestate,x,y)
     int button,mousestate,x,y;
{
  if(state&STATE_GAME)
    switch(mousestate)
      {
      case GLUT_UP:
	accel=-ACCEL;
	break;
      case GLUT_DOWN:
	accel=ACCEL;
	break;
      }
  else if(state&STATE_END)
    {
      if(mousestate==GLUT_DOWN)
	{
	  init();
	  accel=ACCEL;
	}
    }
}

void periodic(value)
     int value;
{
  float newheight,newcenter,w;
  unsigned int k;
  int j;

  speed+=accel;
  newheight=tails[0]+speed;
#ifdef GLSFCAVE_SOUND
  if(se)
    alSourcef(sndsrc,AL_PITCH, 1.0f+newheight);
#endif
  score++;
  w=caveWidth(score);
  memmove(&(tails[1]),tails,sizeof(float)*(NUMTAILS-1));
  memmove(&(center[1]),center,(3*NUMTAILS-1)*sizeof(float));
  memmove(&(pad[1]),pad,(3*NUMTAILS-1)*sizeof(float));
  newcenter=0.05f-0.1f*((float)random())/((float)RANDOM_MAX)+center[1];
  center[0]=MIN(MAX(newcenter,-1.0f+w),1.0f-w);
  tails[0]=newheight;
  pad[0]=(w-2*PADWIDTH)*(2.0f*((float)random())/((float)RANDOM_MAX)-1.0f)+center[0];
  /* Hit detection */
  j=score-2*NUMTAILS;
  k=j%PADFREQ;
  if(tails[0]>1.0f||tails[0]<-1.0f||fabs(tails[0]-center[2*NUMTAILS])>=caveWidth(score-2*NUMTAILS) || (j>0 && k<PADDEPTH && fabs(tails[0]-pad[2*NUMTAILS+k])<PADWIDTH))
    {
      state=STATE_CRASH;
#ifdef GLSFCAVE_SOUND
      if(se)
	alSourceStop(sndsrc);
#endif
      syncHS();
    }
  else
    glutTimerFunc(TIMER_MS,&periodic,0);    
  glutPostRedisplay();
}

void resize(w,h)
     int w,h;
{
  winwidth=w;
  winheight=h;
  glViewport(0,0,w,h);
  glLoadIdentity();
}

unsigned long seedRandom(void)
{
  FILE *fh;
  unsigned long seed=1;
  
  if((fh=fopen(DEVRANDOM,"rb"))==NULL)
    perror("fopen");
  else
    {
      fread(&seed,sizeof(unsigned long),1,fh);
      fclose(fh);
    }
  srandom(seed);
  return seed;

}

float caveWidth(int score)
{
  float width=1.0f;
  if(score>0)
    {
      width=0.2f+4000.0f/(((float) score)+5000.0f);
    }
  return width;
}

void bye()
{
#ifdef GLSFCAVE_SOUND
  alutExit();
#endif
  syncHS();
}

int main(argc,argv)
     int argc;
     char **argv;
{
  char *winname;
  unsigned int i;
  
  /* print version */
  for(i=1; i<argc; i++)
    {
      if(strcmp(argv[i],"-v")==0 || strcmp(argv[i],"--version")==0)
	{
	  printf("GLSFcave " GLSFCAVE_VERSION "\n");
	  exit(EXIT_SUCCESS);
	}
      else if(strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0)
	{
#ifdef GLSFCAVE_SOUND
	  printf("usage: %s [-q | --no-sound] [-v | --version] [-h | --help]\n",argv[0]);
	  printf("\t-q, --no-sound : disable sound(press 'S' to toggle during play)\n");
#else
	  printf("usage: %s [-v | --version] [-h | --help]\n",argv[0]);
#endif
	  printf("\t-v, --version : print version\n");
	  printf("\t-h, --help : show this help\n");
	  exit(EXIT_SUCCESS);
	}
#ifdef GLSFCAVE_SOUND
      else if(strcmp(argv[i],"-q")==0 || strcmp(argv[i],"--no-sound")==0)
	se=0;
#endif
    }

  /* prepare scorefile name */
  winname=getpwuid(getuid())->pw_dir;
  if(strlen(winname)>PATH_MAX-2-strlen(HSFILE))
    {
      strncpy(hsfile,HSFILE,PATH_MAX);
    }
  else
    {
      strcpy(hsfile,winname);
      strcat(hsfile,"/");
      strcat(hsfile,HSFILE);
    }

#ifdef GLSFCAVE_SOUND
  alutInit(&argc,argv);
  sndbuf=alutCreateBufferWaveform(ALUT_WAVEFORM_SQUARE,220,0,1.0f);
  alGenSources(1,&sndsrc);
  alSourcei(sndsrc,AL_BUFFER,sndbuf);
  alSourcei(sndsrc,AL_LOOPING,AL_TRUE);
  alSourcef(sndsrc,AL_GAIN,0.7f);
#endif /* defined(GLSFCAVE_SOUND) */

  glutInitWindowSize(WINDOWSIZE);
  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  glutCreateWindow(((winname=basename(argv[0]))==NULL)?DEFAULTTITLE:winname);
  glutDisplayFunc(&display);
  glutKeyboardFunc(&keyboard);
  glutMouseFunc(&mouse);
  glutReshapeFunc(&resize);
  glClearColor(0.0f,0.0f,0.0f,1.0f);

  seedRandom();
  state=STATE_END;
  highscore=0;
  syncHS();
  atexit(&bye);

  glutMainLoop();
  return EXIT_FAILURE;
}
