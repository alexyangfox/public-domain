#include <stdlib.h>
#include <math.h>
#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
static int redraw_needed=0;
static int koch_loop_count=4;
static const float root3=1.7320504;
static const float size=0.7f;
static void koch2(float startx,float starty,float endx,float endy, int loop);
static void koch(float startx,float starty,float endx,float endy);
static void display(void);
static void reshape(int w,int h);
extern int main(int argc, char **argv);

static void koch2(float startx,float starty,float endx,float endy, int loop)
{
	float px,py,qx,qy,rx,ry,pqx,pqy,prx,pry;
	px=(2.f*startx+endx)/3.f;
	py=(2.f*starty+endy)/3.f;
	qx=(startx+2.f*endx)/3.f;
	qy=(starty+2.f*endy)/3.f;
	pqx=qx-px;
	pqy=qy-py;
	prx=0.5f*pqx-root3/2.*pqy;
	pry=root3/2.*pqx+0.5f*pqy;
	rx=px+prx;
	ry=py+pry;
	if(loop==0)
	{
		glBegin(GL_LINE_STRIP);
		glVertex2f(startx,starty);
		glVertex2f(px,py);
		glVertex2f(rx,ry);
		glVertex2f(qx,qy);
		glVertex2f(endx,endy);
		glEnd();
	} else {
		int nloop=loop-1;
		koch2(startx,starty,px,py,nloop);
		koch2(px,py,rx,ry,nloop);
		koch2(rx,ry,qx,qy,nloop);
		koch2(qx,qy,endx,endy,nloop);
	}

}

static void koch(float startx,float starty,float endx,float endy)
{
	int i;
	float len;
	glClear(GL_COLOR_BUFFER_BIT|GL_ACCUM_BUFFER_BIT);
	glColor3f(1.0f,1.0f,1.0f);
	len=sqrtf((endx-startx)*(endx-startx)+(endy-starty)*(endy-starty));
	for(i=0; i<3; i++)
	{
		glPushMatrix();
		glRotatef((float)(120*i),0.f,0.f,1.f);
		glTranslatef(0.f,len/2./root3,0.f);
		koch2(startx,starty,endx,endy,koch_loop_count);
		glPopMatrix();

	}
	glAccum(GL_ACCUM,1.0f);
}

static void display()
{
	if(redraw_needed)
	{
		koch(-size,0.0,size,0.0);
		redraw_needed=0;
	}
	glClear(GL_COLOR_BUFFER_BIT);
	glAccum(GL_RETURN,1.0);
	glFlush();
}

static void reshape(int w,int h)
{
	glViewport(0,0,w,h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	redraw_needed=1;
}

extern int main(int argc, char **argv)
{
	glutInitWindowSize(512,512);
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_RGBA|GLUT_ACCUM);
	glutCreateWindow("Koch");
	glutDisplayFunc(&display);
	glutReshapeFunc(&reshape);
	glClearColor(0.0f,0.0f,1.0f,0.0f);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);
	if(argc>=2)
	{
		koch_loop_count=strtol(argv[1],NULL,10);
		if(koch_loop_count<0)
			koch_loop_count=0;
	}
	glutMainLoop();
	exit(EXIT_FAILURE);
}


