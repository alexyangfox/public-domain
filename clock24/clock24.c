/**
 * clock24 : 24 hour analog clock
 * -- TOMARI, Hisanobu
 */
#define VERSION "1.0"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#else
#include <GL/glut.h>
#endif

#define SHADOW_DISTANCE 0.03f
#define LONGHAND_HEIGHT 0.93f
#define LONGHAND_WIDTH  0.05f
#define SHORTHAND_HEIGHT 0.60f
#define SHORTHAND_WIDTH 0.07f
#define BGCOLOR_NORMAL 0.24f,0.24f,0.24f,0.0f
#define BGCOLOR_ALARM 1.0f,0.0f,0.0f,0.0f

typedef enum {
  ClkTyp24 = 1,
  ClkTyp12 = -1 } ClockType;

enum { MenuAlarm,
       MenuToggleReverse, 
       MenuToggleGMT,
       MenuToggleCompat,
       MenuQuit };

/* program states */
static ClockType clktyp=ClkTyp24;
static float forecolor[3]={0.875f,0.875f,0.875f};
static float shadowcolor[3]={0.078f,0.078f,0.078f};
static float handcolor[3]={0.26f,0.26f,0.26f};
static float alarmcolor[3]={1.0f,0.0f,0.0f};
static int reversemode=0;
static int gmtmode=0;
static int width=150,height=150;
static int alarmOn=0;
static int alarmAngle=0;

static void drawBackPanel(void);
static void drawHand(GLenum type,float width,float height);
static void drawHandWithShadow(float angle,float width,float height);
static void drawLineWithShadow(float angle,float width,float height,float *color);
static void display(void);
static void timer(int cond);
static void menuSelect(int val);
static void reshape(int newwidth,int newheight);
static void mouse(int button,int state,int x,int y);
static void motion(int x,int y);
int main(int argc, char **argv);

static void drawBackPanel()
{
  unsigned int i;
  unsigned int step;

  if(clktyp==ClkTyp24)
    step=15; /* 360/24 */
  else if(clktyp==ClkTyp12)
    step=30; /* 360/12 */
  else 
    step=30; /* should not happen; fallback */

  glPushMatrix();
  glColor3fv(forecolor);
  for(i=0; i<360; i+=step)
    {
      float width;
      if(i%90==0)
	width=0.04f;
      else if(i%45==0)
	width=0.03f;
      else
	width=0.02f;

      glBegin(GL_TRIANGLE_FAN);
	  glVertex2f(-width,0.82f);
	  glVertex2f(width,0.82f);
	  glVertex2f(width,0.9f);
	  glVertex2f(-width,0.9f);
      glEnd();
      glRotatef((float)step,0.0f,0.0f,1.0f);
    }
  glPopMatrix();
}

/* type should be one of : GL_TRIANGLE_FAN, GL_LINES_LOOP, GL_QUADS */
static void drawHand(GLenum type,float width,float height)
{
  glBegin(type);
  glVertex2f(0.0f,-0.1f);
  glVertex2f(-width,0.0f);
  glVertex2f(0.0f,height);
  glVertex2f(width,0.0f);
  glEnd();
}

static void drawHandWithShadow(float angle,float width,float height)
{
  /* shadow */
  glPushMatrix();
  glTranslatef(SHADOW_DISTANCE,-SHADOW_DISTANCE,0.0f);
  glRotatef(angle,0.0f,0.0f,-1.0f);
  glColor3fv(shadowcolor);
  drawHand(GL_TRIANGLE_FAN,width,height);
  glPopMatrix();

  glPushMatrix();
  glRotatef(angle,0.0f,0.0f,-1.0f);
  /* hand body */
  glColor3fv(handcolor);
  drawHand(GL_TRIANGLE_FAN,width,height);
  /* border */
  glColor3fv(forecolor);
  drawHand(GL_LINE_LOOP,width,height);
  glPopMatrix();
}

static void drawLineWithShadow(float angle,float width,float height,float *color)
{
  glLineWidth(width);
  glPushMatrix();
  glTranslatef(SHADOW_DISTANCE,-SHADOW_DISTANCE,0.0f);
  glRotatef((float)angle,0.0f,0.0f,-1.0f);
  glColor3fv(shadowcolor);
  glBegin(GL_LINES);
  glVertex2f(0.0f,-0.1f);
  glVertex2f(0.0f,height);
  glEnd();
  glPopMatrix();
  glColor3fv(color);
  glPushMatrix();
  glRotatef((float)angle,0.0f,0.0f,-1.0f);
  glBegin(GL_LINES);
  glVertex2f(0.0f,-0.1f);
  glVertex2f(0.0f,height);
  glEnd();
  glPopMatrix();
}

static void display()
{
  time_t timev;
  struct tm tmval;
  unsigned int lh_angle,sh_angle,sec_angle;
  /* calc angle */
  timev=time(NULL);
  if(gmtmode)
    gmtime_r(&timev,&tmval);
  else
    localtime_r(&timev, &tmval);

  sec_angle=tmval.tm_sec*6;
  lh_angle=tmval.tm_min*6;

  if(clktyp==ClkTyp24)
    sh_angle=15*tmval.tm_hour+tmval.tm_min/4;
  else
    sh_angle=30*(tmval.tm_hour%12)+tmval.tm_min/2;
  if(reversemode)
    sh_angle=sh_angle+180;

  if(alarmOn==1 && sh_angle==alarmAngle) /* compare integers */
    {
      glClearColor(BGCOLOR_ALARM);
      alarmOn=2;
    }

  /* draw */
  glLoadIdentity();
  glClear(GL_COLOR_BUFFER_BIT);
  drawBackPanel();
  if(alarmOn==1 || alarmOn==3)
    drawLineWithShadow(alarmAngle,3.0f,SHORTHAND_HEIGHT,alarmcolor);
  glLineWidth(1.0f);
  drawHandWithShadow(sh_angle,SHORTHAND_WIDTH,SHORTHAND_HEIGHT); /* hour */
  drawHandWithShadow(lh_angle,LONGHAND_WIDTH,LONGHAND_HEIGHT);   /* min */
  drawLineWithShadow(sec_angle,1.5f,LONGHAND_HEIGHT,forecolor);  /* sec */

  glutSwapBuffers();
}

static void timer(int cond)
{
  if(cond)
    glutTimerFunc(500,&timer,1); /* niquist freq for 1Hz */
  glutPostRedisplay();
}

static void menuSelect(int val)
{
  switch(val)
    {
    case MenuAlarm:
      if(!alarmOn)
	{
	  alarmOn=3;
	}
      else
	alarmOn=0;
      break;
    case MenuToggleReverse:
      reversemode=!reversemode;
      break;
    case MenuToggleGMT:
      gmtmode=!gmtmode;
      break;
    case MenuToggleCompat:
      clktyp=-clktyp;
      break;
    case MenuQuit:
      exit(EXIT_SUCCESS);
      break;
    }
  glutPostRedisplay();
}

static void reshape(int newwidth,int newheight)
{
  width=newwidth;
  height=newheight;
  glViewport(0,0,width,height);
  glutPostRedisplay();
}

static void mouse(int button,int state,int x,int y)
{
  if(button==GLUT_LEFT_BUTTON)
    {
      if(state==GLUT_DOWN && alarmOn==2)
	{
	  glClearColor(BGCOLOR_NORMAL);
	  alarmOn=0;
	}
    }
}

static void motion(int x,int y)
{
  if(alarmOn>0)
    {
      float nx,len;
      int xa;
      x=x-width/2;
      y=y-height/2;
      len=sqrtf(x*x+y*y);
      if(!isnormal(len))
	return;
      nx=((float)x)/len; /* FDIV */
      if(!isnormal(nx))
	return;
      xa=(int) (asinf(nx)*180.0f/M_PI); /* FDIV, in degrees */
      if(y>0)
	xa=180-xa;
      if(xa<0)
	xa=360+xa;
      alarmAngle=xa;
      alarmOn=1;
      glClearColor(BGCOLOR_NORMAL);
      glutPostRedisplay();
    }
}

int main(int argc, char **argv)
{
  int opt;
  char *title="clock24";
  while((opt=getopt(argc,argv,"crghvt:"))!=-1)
    {
      switch(opt) 
	{
	case 'c':
	  clktyp=ClkTyp12;
	  break;
	case 'r':
	  reversemode=1;
	  break;
	case 'g':
	  gmtmode=1;
	  break;
	case 'h':
	  printf("clock24 " VERSION " -- 24 hour analog clock\n");
	  printf("usage: %s [-c] [-r] [-g] [-t title] [-h]\n",argv[0]);
	  printf("\t-c : compatibility mode (12 hour clock mode)\n");
	  printf("\t-r : reverse mode (lower half represents night)\n");
	  printf("\t-g : GMT mode (shows GMT instead of local time)\n");
	  printf("\t-t : set title\n");
	  printf("\t-h : shows this help\n");
	  printf("\tOther GLUT options like -display and -geometry apply.\n");
	  exit(EXIT_SUCCESS);
	case 't':
	  title=optarg;
	  break;
	}
    }
  glutInitWindowSize(width,height);
  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  glutCreateWindow("clock24");
  glutDisplayFunc(&display);
  glutReshapeFunc(&reshape);
  glutMouseFunc(&mouse);
  glutMotionFunc(&motion);
  glClearColor(0.24f,0.24f,0.24f,0.0f);

  /* enable antialiasing */
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POLYGON_SMOOTH);
  glEnable (GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);

  /* Create a pop-up menu */
  glutCreateMenu(menuSelect);
  glutAddMenuEntry("Alarm",MenuAlarm);
  glutAddMenuEntry("Toggle reverse mode",MenuToggleReverse);
  glutAddMenuEntry("Toggle display GMT",MenuToggleGMT);
  glutAddMenuEntry("Toggle compatibility mode",MenuToggleCompat);
  glutAddMenuEntry("Quit",MenuQuit);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  glutTimerFunc(500,&timer,1);
  glutMainLoop();
  return EXIT_FAILURE;
}
